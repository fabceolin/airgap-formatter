# ADR 001: AsyncSerialiser Pattern for WASM Async Operations

## Status

Accepted

## Date

2026-01-26

## Context

Qt WASM applications using Emscripten's Asyncify mechanism crash when multiple async operations (val::await) execute concurrently. The error "Cannot have multiple async operations in flight at once" indicates the single-buffer limitation of Asyncify's stack saving mechanism.

### Problem Statement

When a user performs rapid operations (e.g., clicking multiple history entries, format-then-copy, paste-and-format), the application crashes with:
```
Aborted(Assertion failed: Cannot have multiple async operations in flight at once)
RuntimeError: memory access out of bounds
BindingError: Cannot use deleted val. handle = 0
```

### Affected Operations

All JavaScript Promise-based operations require Asyncify:
- `formatJson()` - Rust WASM parsing/formatting
- `minifyJson()` - Rust WASM minification
- `validateJson()` - Rust WASM validation
- `saveToHistory()` - IndexedDB write
- `loadHistory()` - IndexedDB read
- `copyToClipboard()` - navigator.clipboard.writeText()
- `readFromClipboard()` - navigator.clipboard.readText()

## Decision

Implement a C++ singleton `AsyncSerialiser` class that:
1. Queues all Asyncify-dependent operations
2. Executes them sequentially (FIFO)
3. Uses QFuture/QPromise for completion signaling
4. Includes watchdog timeout for hung operations
5. Isolates errors so one failure doesn't block the queue

### Implementation Details

```cpp
class AsyncSerialiser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int queueLength READ queueLength NOTIFY queueLengthChanged)

public:
    using AsyncTask = std::function<QFuture<QVariant>()>;

    static AsyncSerialiser& instance();
    void enqueue(const QString& taskName, AsyncTask task);
    void clearQueue();
    int queueLength() const;

signals:
    void taskStarted(const QString& taskName);
    void taskCompleted(const QString& taskName, bool success);
    void taskTimedOut(const QString& taskName);
    void taskRejected(const QString& taskName);
    void queueLengthChanged(int length);
    void queueLengthWarning(int length);

private:
    void processNext();

    std::queue<std::pair<QString, AsyncTask>> m_queue;
    bool m_isBusy = false;
    static constexpr int MAX_QUEUE_SIZE = 100;
    static constexpr int QUEUE_WARNING_THRESHOLD = 10;
    static constexpr int WATCHDOG_TIMEOUT_MS = 30000;
};
```

### Usage Pattern

```cpp
void JsonBridge::formatJson(const QString& input, const QString& indent)
{
    AsyncSerialiser::instance().enqueue("formatJson", [this, input, indent]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        try {
            // Perform async operation (Asyncify suspension point)
            val result = jsFormatFunction.call<val>("format", ...).await();
            emit formatCompleted(convertResult(result));
            promise.addResult(QVariant(true));
        } catch (...) {
            emit formatCompleted({{"success", false}, {"error", "..."}});
            promise.addResult(QVariant(false));
        }

        promise.finish();
        return future;
    });
}
```

## Consequences

### Positive

- **Eliminates crashes** - No more "multiple async operations" errors
- **Deterministic execution** - FIFO ordering makes behavior predictable
- **Centralized management** - Single point of control for all async operations
- **Easy extensibility** - New async operations just call `enqueue()`
- **Error isolation** - One failing task doesn't block others
- **Observability** - Signals provide insight into queue state

### Negative

- **Sequential execution** - Operations cannot run in parallel (no performance gain from parallelism)
- **Added latency** - Queued operations wait for previous ones to complete
- **Complexity** - Signal-based API is more complex than synchronous returns
- **Refactoring required** - All async call sites needed migration

### Neutral

- **Memory overhead** - Queue stores pending tasks (bounded by MAX_QUEUE_SIZE)
- **Future migration** - JSPI would simplify but require similar refactoring

## Alternatives Considered

### 1. JSPI (JavaScript Promise Integration)

**Pros:**
- Native stack switching
- Supports concurrent async operations
- Better performance potential

**Cons:**
- Limited browser support (Chrome only as of early 2026)
- Requires Emscripten flag change and testing
- Not backward compatible

**Verdict:** Deferred for future Story 5.4 when browser support improves.

### 2. JavaScript Orchestration (Inversion of Control)

**Pros:**
- No Asyncify overhead
- True parallelism in JS layer
- Browser-native async/await

**Cons:**
- Splits business logic between C++ and JavaScript
- Harder to maintain and debug
- Requires rewriting significant C++ code

**Verdict:** Too invasive for the current architecture.

### 3. QML Timer Delays (Original Workaround)

**Pros:**
- Simple to implement
- No C++ changes needed

**Cons:**
- Fragile timing assumptions
- Race conditions on slow devices
- Non-deterministic behavior
- Doesn't scale with more async operations

**Verdict:** Temporary workaround, replaced by AsyncSerialiser.

### 4. Web Workers

**Pros:**
- True parallelism
- Isolates async work from main thread

**Cons:**
- Cannot share WASM memory easily
- Complex message passing
- Significant architectural change

**Verdict:** Overkill for the problem scope.

## Related

- `docs/ASYNCIFY-CONFLICT-REPORT.md` - Original issue documentation
- `qt/asyncserialiser.h` / `.cpp` - Implementation
- `qt/tests/tst_asyncserialiser.cpp` - Unit tests
- Story 5.1 - AsyncSerialiser implementation
- Story 5.2 - JsonBridge migration
- Story 5.3 - QML workaround removal
- Story 5.4 - Future JSPI migration (planned)

## References

- [Emscripten Asyncify Documentation](https://emscripten.org/docs/porting/asyncify.html)
- [WebAssembly JSPI Proposal](https://github.com/aspect-build/aspect-cli/issues/1138)
- [Qt for WebAssembly](https://doc.qt.io/qt-6/wasm.html)
