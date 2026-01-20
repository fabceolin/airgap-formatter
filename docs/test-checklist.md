# Cross-Browser Test Checklist

Use this checklist for each browser when testing releases.

## Test Information

| Field | Value |
|-------|-------|
| Browser | |
| Version | |
| Date | |
| Tester | |
| Build/Commit | |

---

## Core Functionality

- [ ] App loads without errors
- [ ] WASM module initializes (check console for "[Airgap] Rust WASM module loaded")
- [ ] Input pane accepts text
- [ ] Format button produces prettified JSON
- [ ] Minify button produces minified JSON
- [ ] Copy button copies output to clipboard (verify paste works)
- [ ] Clear button clears both panes
- [ ] Indentation dropdown changes format output

## Validation

- [ ] Valid JSON shows green "Valid JSON" status
- [ ] Invalid JSON shows red "Invalid JSON" status
- [ ] Error message shows line and column number
- [ ] Error line is highlighted in input pane
- [ ] Statistics display (Objects, Arrays, Keys, Depth) for valid JSON

## Keyboard Navigation

- [ ] Tab navigates between focusable elements
- [ ] Shift+Tab navigates backwards
- [ ] Enter/Space activates buttons
- [ ] Focus indicators are visible on all focusable elements

## Keyboard Shortcuts

- [ ] Ctrl+Shift+F formats JSON
- [ ] Ctrl+Shift+M minifies JSON
- [ ] Ctrl+Shift+C copies output

## Accessibility

- [ ] Focus indicators visible on buttons
- [ ] Focus indicators visible on dropdown
- [ ] Focus indicators visible on text areas
- [ ] Color contrast is sufficient (text readable)

## Performance

- [ ] 1MB JSON formats in <100ms

**Performance Test Script:**
```javascript
// Run in DevTools Console after initBridge()
async function testPerformance() {
    const generateLargeJson = (targetSizeKB) => {
        const items = [];
        const count = (targetSizeKB * 1024) / 100;
        for (let i = 0; i < count; i++) {
            items.push({
                id: i,
                name: `Item ${i}`,
                description: `Description for item ${i}`,
                value: Math.random() * 1000,
                active: i % 2 === 0
            });
        }
        return JSON.stringify({ items });
    };

    const largeJson = generateLargeJson(1024);
    console.log('Test JSON size:', (largeJson.length / 1024).toFixed(2), 'KB');

    const start = performance.now();
    const result = JsonBridge.formatJson(largeJson, 'spaces:4');
    const duration = performance.now() - start;

    console.log('Format duration:', duration.toFixed(2), 'ms');
    console.log('Status:', duration < 100 ? 'PASS' : 'FAIL');
    return { duration, pass: duration < 100 };
}

await testPerformance();
```

## Offline Mode

- [ ] Service Worker registers (check DevTools > Application > Service Workers)
- [ ] Assets cached (check DevTools > Application > Cache Storage)
- [ ] App loads when offline (enable "Offline" in Network tab, reload)
- [ ] Format/Minify work offline
- [ ] Copy to clipboard works offline

## Console Verification

- [ ] No errors during normal use
- [ ] Airgap mode logs present: "[Airgap] Mode Active"
- [ ] No network requests after initial load (check Network tab)
- [ ] No localStorage/sessionStorage usage (check Application tab)

## Trust Indicators

- [ ] "Airgap Protected" badge visible in header
- [ ] "Offline Ready" indicator visible after load
- [ ] Privacy footer text visible in status bar
- [ ] Tooltips appear on badge hover

## Responsive Layout

- [ ] Horizontal split at >= 1200px width
- [ ] Vertical split at < 1200px width
- [ ] Split handle is draggable
- [ ] Both panes have minimum sizes

---

## Issues Found

| Issue | Severity | Description |
|-------|----------|-------------|
| | | |
| | | |
| | | |

## Notes

_______________________________________________________

_______________________________________________________

_______________________________________________________

---

## Browser Compatibility Matrix

| Feature | Chrome 90+ | Firefox 88+ | Safari 14+ | Edge 90+ |
|---------|------------|-------------|------------|----------|
| WASM | ✓ | ✓ | ✓ | ✓ |
| Service Worker | ✓ | ✓ | ✓ | ✓ |
| Clipboard API | ✓ | ✓ | ✓ (gesture) | ✓ |
| CSS Grid | ✓ | ✓ | ✓ | ✓ |

## Known Browser Quirks

### Safari
- Clipboard API may require explicit user gesture
- WASM streaming may behave differently

### Firefox
- Service Worker scope handling differs slightly
- WASM instantiation may be marginally slower

### Edge
- Generally matches Chrome behavior (Chromium-based)
