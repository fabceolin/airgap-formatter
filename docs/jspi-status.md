# JSPI (JavaScript Promise Integration) Browser Support Status

**Last Updated:** 2026-01-26

## What is JSPI?

JSPI (JavaScript Promise Integration) is a WebAssembly proposal that allows Wasm code to suspend execution when calling JavaScript async functions and resume when the Promise resolves. Unlike Asyncify (our current solution), JSPI supports **multiple concurrent suspensions** because the browser VM handles stack management natively.

### Benefits over Asyncify

| Aspect | Asyncify | JSPI |
|--------|----------|------|
| Concurrent suspensions | Single only | Multiple supported |
| Binary size overhead | ~30-50% larger | Minimal |
| Runtime overhead | Stack copy/restore | Native VM support |
| Complexity | Build-time instrumentation | Runtime support |

## Current Browser Support

| Browser | Version | Status | Notes |
|---------|---------|--------|-------|
| Chrome | 137+ | Enabled by default | Full JSPI support |
| Edge | 137+ | Enabled by default | Chromium-based, same as Chrome |
| Firefox | 130+ | Behind flag | Enable `javascript.options.wasm_jspi` in about:config |
| Safari | - | Not supported | No timeline announced |

### Detection API

JSPI support is detected via:
1. `WebAssembly.Suspending` constructor (Chrome 137+)
2. `WebAssembly.Function` with `suspending: 'first'` option (older implementations)

## Current Status in Airgap JSON Formatter

**Production Default:** Asyncify (broad compatibility)

The application uses the `AsyncSerialiser` pattern to serialize async operations, preventing concurrent Asyncify suspensions. This works on all browsers.

**JSPI Preparation:**
- Feature detection (`jspi-detect.js`) runs at startup
- `window.JSPI_AVAILABLE` boolean exposed for runtime checks
- CMake `ENABLE_JSPI` option available for experimental builds
- AsyncSerialiser has prepared (commented) bypass code for future activation

## How to Test JSPI Build

### Prerequisites

- Chrome 137+ or Edge 137+ (or Firefox 130+ with flag enabled)
- Docker development environment

### Build with JSPI

```bash
# Enter Docker container
docker run -it --rm -v $(pwd):/workspace airgap-formatter

# Build with JSPI enabled
cd qt
mkdir -p build-jspi && cd build-jspi
${QT_WASM_PATH}/bin/qt-cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_JSPI=ON
cmake --build . --parallel $(nproc)

# Copy outputs
cp *.wasm *.js ../../dist-jspi/
```

### Verify JSPI Detection

1. Open browser DevTools console
2. Load the application
3. Check console for: `[JSPI Detection] JSPI available: true`
4. Verify: `window.JSPI_AVAILABLE === true`

### Expected Behavior

With JSPI build in a supported browser:
- JSPI detection returns `true`
- Application functions normally (same user experience)
- Future: When bypass is enabled, operations execute without queuing

## Monitoring Checklist

Review quarterly for browser support updates:

- [ ] **WebAssembly JSPI Proposal Status**
  - GitHub: https://github.com/aspect-build/aspect-cli/issues/1138
  - Spec: https://github.com/aspect-build/aspect-cli/issues/1138

- [ ] **Chrome Platform Status**
  - https://chromestatus.com/ (search "JSPI")
  - Current: Enabled by default in Chrome 137+

- [ ] **Firefox JSPI Flag Status**
  - https://bugzilla.mozilla.org/ (search "WebAssembly JSPI")
  - Current: Behind `javascript.options.wasm_jspi` flag

- [ ] **Safari WebKit Status**
  - https://webkit.org/status/
  - Current: Not listed / no timeline

- [ ] **Can I Use**
  - https://caniuse.com/ (search "WebAssembly JSPI")
  - Check for updated browser support data

## Migration Plan

When Safari announces JSPI support (estimated: TBD), we will:

### Phase 1: Parallel Support
1. Enable JSPI runtime detection to choose execution path
2. Uncomment the JSPI bypass code in `AsyncSerialiser::enqueue()`
3. Test thoroughly across all supported browsers
4. Keep Asyncify as fallback for older browsers

### Phase 2: JSPI Default
1. Make JSPI build the default (`ENABLE_JSPI=ON`)
2. Keep Asyncify fallback for Safari/older browsers
3. Measure performance improvements (binary size, execution speed)

### Phase 3: Asyncify Removal (Long-term)
1. When all major browsers support JSPI (including Safari)
2. Remove Asyncify build option
3. Simplify AsyncSerialiser to thin wrapper or remove entirely
4. Expected benefits:
   - ~50% reduction in WASM binary size
   - Faster startup time
   - Simpler codebase

## Technical References

- [WebAssembly JSPI Proposal](https://github.com/aspect-build/aspect-cli/issues/1138)
- [Emscripten JSPI Documentation](https://emscripten.org/docs/porting/asyncify.html#jspi)
- [Chrome JSPI Implementation](https://chromestatus.com/)
- [V8 Blog: WebAssembly JSPI](https://v8.dev/)

## Version History

| Date | Change | Author |
|------|--------|--------|
| 2026-01-26 | Initial documentation created | Dev Agent (Story 5.4) |
