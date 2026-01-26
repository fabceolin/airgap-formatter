/**
 * @file jspi-detect.js
 * @brief JSPI (JavaScript Promise Integration) feature detection for WebAssembly
 *
 * Detects if the browser supports JSPI, which allows Wasm to suspend and resume
 * with multiple concurrent suspensions. Unlike Asyncify, JSPI handles stack
 * management natively in the browser VM.
 *
 * JSPI eliminates the single-flight limitation of Asyncify, potentially allowing
 * the AsyncSerialiser queue to be bypassed when JSPI is available.
 */
(function() {
    'use strict';

    let jspiAvailable = false;

    try {
        // Primary check: WebAssembly.Suspending constructor
        // This is the main indicator of JSPI availability (Chrome 137+)
        if (typeof WebAssembly.Suspending === 'function') {
            jspiAvailable = true;
            console.log('[JSPI Detection] Detected via WebAssembly.Suspending');
        }

        // Alternative check for older Chrome versions with experimental flag
        // WebAssembly.Function with suspending option
        if (!jspiAvailable && typeof WebAssembly.Function === 'function') {
            try {
                // Attempt to create a function with suspending option
                // This will throw if JSPI is not supported
                const testFunc = new WebAssembly.Function(
                    { parameters: [], results: [] },
                    () => {},
                    { suspending: 'first' }
                );
                if (testFunc !== undefined) {
                    jspiAvailable = true;
                    console.log('[JSPI Detection] Detected via WebAssembly.Function suspending option');
                }
            } catch (e) {
                // WebAssembly.Function exists but doesn't support suspending option
                // This is expected on browsers without JSPI
            }
        }
    } catch (e) {
        // Feature detection failed - default to false (Asyncify path)
        console.log('[JSPI Detection] Detection error (falling back to Asyncify):', e.message);
        jspiAvailable = false;
    }

    // Expose result globally for both JavaScript and C++ access
    window.JSPI_AVAILABLE = jspiAvailable;

    // Log browser info for debugging
    const browserInfo = navigator.userAgent;
    console.log('[JSPI Detection] JSPI available:', jspiAvailable);
    console.log('[JSPI Detection] Browser:', browserInfo);

    // Expose a function for programmatic access
    window.isJspiAvailable = function() {
        return window.JSPI_AVAILABLE;
    };
})();
