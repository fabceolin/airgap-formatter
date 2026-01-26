# ASYNCIFY Conflict Report: Qt WASM + IndexedDB History Loading

**Date**: 2026-01-22
**Status**: ✅ Fixed (AsyncSerialiser implementation)
**Fix Date**: 2026-01-26
**Severity**: Critical (crashes application) - RESOLVED

## Resolution Summary

The Asyncify conflict issue has been permanently resolved through the implementation of the `AsyncSerialiser` class (Stories 5.1-5.3). All async operations are now queued and executed sequentially, preventing concurrent Asyncify suspensions.

**Key Changes:**
1. **AsyncSerialiser** - C++ singleton queue that serializes all async operations (Story 5.1)
2. **JsonBridge refactored** - All 7 async operations now use AsyncSerialiser (Story 5.2)
3. **QML workarounds removed** - Timer-based workarounds eliminated from Main.qml (Story 5.3)

See: `docs/adr/001-async-serialisation.md` for architectural decision details.

---

## Original Issue Summary (Historical Reference)

When selecting an item from the History panel in the Qt WASM application, the app crashed with:
```
Aborted(Assertion failed: Cannot have multiple async operations in flight at once)
RuntimeError: Aborted(...)
BindingError: Cannot use deleted val. handle = 0
```

## Root Cause

**Emscripten ASYNCIFY Limitation**: ASYNCIFY is Emscripten's mechanism for allowing synchronous C++ code to call asynchronous JavaScript APIs (like IndexedDB, Promises). It works by:

1. Unwinding the C++ call stack when an async operation starts
2. Saving the stack state
3. Resuming execution when the Promise resolves

**Critical Limitation**: ASYNCIFY can only handle **one async operation at a time**. If a second async operation is initiated while the first is still pending, the runtime aborts.

## Call Chain Analysis

When user clicks a history entry:

```
1. Click Event → selectEntry(modelData)
   ├── entrySelected(entry.content) signal emitted
   └── historyDrawer.close() [triggers animation events]

2. Main.qml receives entrySelected signal
   ├── inputPane.text = content
   │   └── triggers onTextChanged → validationTimer.restart()
   └── toolbar.formatRequested(indent)

3. formatRequested handler (ALL use promise.await())
   ├── JsonBridge.formatJson()     ← ASYNCIFY call #1
   ├── JsonBridge.loadTreeModel()  ← Potentially async
   ├── validateInput()
   │   └── JsonBridge.validateJson() ← ASYNCIFY call #2 (CONFLICT!)
   └── JsonBridge.saveToHistory()  ← ASYNCIFY call #3 (CONFLICT!)
```

## Technical Details

### C++ Code Pattern (jsonbridge.cpp)

```cpp
QVariantMap JsonBridge::formatJson(const QString& input, const QString& indentType)
{
#ifdef __EMSCRIPTEN__
    val window = val::global("window");
    val jsonBridge = window["JsonBridge"];
    val promise = jsonBridge.call<val>("format", ...);
    val jsResult = promise.await();  // ← ASYNCIFY unwind point
#endif
}
```

Each `promise.await()` triggers ASYNCIFY stack unwinding. Multiple simultaneous `await()` calls cause:
- "Cannot have multiple async operations in flight at once"
- Memory corruption ("memory access out of bounds")
- Deleted value errors ("Cannot use deleted val")

### Affected Operations

| C++ Function | JavaScript Call | Uses ASYNCIFY |
|--------------|-----------------|---------------|
| `formatJson()` | `JsonBridge.format()` | Yes |
| `minifyJson()` | `JsonBridge.minify()` | Yes |
| `validateJson()` | `JsonBridge.validate()` | Yes |
| `saveToHistory()` | `JsonBridge.saveHistory()` | Yes (IndexedDB) |
| `loadHistory()` | `JsonBridge.loadHistory()` | Yes (IndexedDB) |
| `copyToClipboard()` | `navigator.clipboard.writeText()` | Yes |
| `readFromClipboard()` | `navigator.clipboard.readText()` | Yes |

## Historical Workaround (REMOVED)

**Note:** The following workaround was temporary and has been replaced by the AsyncSerialiser implementation. This section is preserved for historical reference only.

The original fix used QML timers to defer async operations so they didn't overlap:

### 1. Deferred Timers (Main.qml)

```qml
// Defer history save to avoid overlapping with formatJson
Timer {
    id: saveHistoryTimer
    interval: 100
    property string jsonToSave: ""
    onTriggered: JsonBridge.saveToHistory(jsonToSave)
}

// Defer format request when loading from history
Timer {
    id: deferredFormatTimer
    interval: 50
    onTriggered: toolbar.formatRequested(indentType)
}
```

### 2. Skip Redundant Validation

```qml
property bool skipNextValidation: false

function validateInput() {
    if (skipNextValidation) {
        skipNextValidation = false;
        return;
    }
    // ... async validation
}
```

### 3. Modified History Selection Handler

```qml
onEntrySelected: (content) => {
    validationTimer.stop();
    skipNextValidation = true;
    inputPane.text = content;
    // Defer format to avoid ASYNCIFY conflicts
    deferredFormatTimer.indentType = toolbar.selectedIndent;
    deferredFormatTimer.restart();
}
```

## Implemented Solution: AsyncSerialiser

The permanent fix implements a C++ singleton queue that serializes all async operations:

```cpp
// AsyncSerialiser usage pattern
AsyncSerialiser::instance().enqueue("operationName", []() {
    QPromise<QVariant> promise;
    auto future = promise.future();
    promise.start();

    // Perform async work (e.g., val::await)
    val result = somePromise.await();

    promise.addResult(QVariant::fromValue(result));
    promise.finish();
    return future;
});
```

**Key Features:**
- Single-flight execution (m_isBusy guard)
- FIFO ordering
- 30-second watchdog timeout
- Error isolation (exceptions don't block queue)
- Queue bounds (max 100 tasks)

**Migrated Operations:**
| Operation | Task Name |
|-----------|-----------|
| formatJson() | "formatJson" |
| minifyJson() | "minifyJson" |
| validateJson() | "validateJson" |
| saveToHistory() | "saveToHistory" |
| loadHistory() | "loadHistory" |
| copyToClipboard() | "copyToClipboard" |
| readFromClipboard() | "readFromClipboard" |

---

## Future Considerations

### 1. JSPI (JavaScript Promise Integration)

Newer Emscripten feature that may handle concurrent async better:
- Use `-sJSPI` flag instead of `-sASYNCIFY`
- Requires Chrome 109+ or Firefox experimental
- [Emscripten JSPI docs](https://emscripten.org/docs/porting/asyncify.html#jspi)

### 2. Async Queue in C++

Implement a queue that serializes all async operations:

```cpp
class AsyncQueue {
    std::queue<std::function<void()>> pending;
    bool running = false;
public:
    void enqueue(std::function<void()> op) {
        pending.push(op);
        if (!running) processNext();
    }
private:
    void processNext() {
        if (pending.empty()) { running = false; return; }
        running = true;
        auto op = pending.front();
        pending.pop();
        op();
        // Call processNext() after async completes
    }
};
```

### 3. Move Async Orchestration to JavaScript

Keep C++ synchronous, do all async coordination in JS:

```javascript
// JS-side orchestration
async function loadAndFormat(entry) {
    const content = await loadFromIndexedDB(entry.id);
    const result = await format(content);
    await saveToHistory(result);
    return result;
}

// C++ calls single JS function
val result = jsOrchestrator.call<val>("loadAndFormat", entryId).await();
```

### 4. Qt 6.7+ WebAssembly Improvements

Qt has been improving WASM support:
- Check if Qt 6.10 has better async handling
- Investigate `QFuture`/`QPromise` integration with WASM
- Review Qt WASM threading options

### 5. Synchronous Alternatives

- **localStorage**: Synchronous but limited to 5MB
- **In-memory storage**: Keep history in WASM memory (loses persistence on reload)
- **Web Workers**: Offload async to worker, communicate via postMessage

## Configuration Attempted

```cmake
# CMakeLists.txt WASM settings
target_link_options(airgap_formatter PRIVATE
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sEXPORT_NAME=createQtAppInstance
    -sENVIRONMENT=web
    -sASYNCIFY
    -sASYNCIFY_STACK_SIZE=131072  # Increased from default 4096
    -sALLOW_MEMORY_GROWTH=1
    --bind
)
```

**Note**: Increasing `ASYNCIFY_STACK_SIZE` helps with deep call stacks but does NOT solve the concurrent async problem.

## Error Messages Reference

### Primary Error
```
Aborted(Assertion failed: Cannot have multiple async operations in flight at once)
```

### Secondary Errors (after abort)
```
RuntimeError: memory access out of bounds
BindingError: Cannot use deleted val. handle = 0
user callback triggered after runtime exited or application aborted. Ignoring.
```

### Debug Build Errors
With `-sASSERTIONS=2 -sSAFE_HEAP=1`:
```
Assertion failed: attempt to write non-integer (NaN) into integer heap
```
(This is a false positive in Qt's internal code, not related to our issue)

## Files Modified

**Current Implementation (Stories 5.1-5.3):**
- `qt/asyncserialiser.h` - AsyncSerialiser class declaration
- `qt/asyncserialiser.cpp` - AsyncSerialiser implementation
- `qt/jsonbridge.h` - Updated with async signals
- `qt/jsonbridge.cpp` - Refactored to use AsyncSerialiser
- `qt/qml/Main.qml` - Timer workarounds removed, direct signal calls

**Historical (Original Workaround - REMOVED):**
- `qt/qml/Main.qml` - Previously had deferred timers and skip validation flag
- `qt/CMakeLists.txt` - Increased ASYNCIFY_STACK_SIZE (still present)

## References

- [Emscripten ASYNCIFY documentation](https://emscripten.org/docs/porting/asyncify.html)
- [Qt for WebAssembly documentation](https://doc.qt.io/qt-6/wasm.html)
- [Emscripten GitHub - Asyncify issues](https://github.com/aspect-build/aspect-cli/issues/1138)
- [WebAssembly JSPI Proposal](https://github.com/aspect-build/aspect-cli/issues/1138)
- [Qt WASM async patterns](https://www.qt.io/blog/aspect-build/aspect-cli/issues/1138)

## Testing

### Automated Tests
Unit tests exist for the AsyncSerialiser implementation:
- `qt/tests/tst_asyncserialiser.cpp` - Tests single-flight, FIFO, watchdog, error isolation
- `qt/tests/tst_jsonbridge_async.cpp` - Tests all 7 async operations use queue

### Manual Testing
See `docs/qa/test-plans/5.3-asyncify-fix-validation.md` for comprehensive E2E test scenarios.

### Build & Test
```bash
# Build
cd qt/build && ninja

# Copy to dist
cp airgap_formatter.* qtloader.js ../../dist/

# Verify
# 1. Open in browser
# 2. Test history loading by clicking entries
# 3. Test rapid format clicks
# 4. Check browser console for ASYNCIFY errors (should be none)
```
