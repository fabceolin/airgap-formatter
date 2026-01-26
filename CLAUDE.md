# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Airgap JSON Formatter is a security-first, client-side JSON manipulation tool. It combines Rust for core logic, Qt for the GUI, and WebAssembly for browser delivery. The key architectural constraint is **zero network communication** after initial load - all processing happens locally in the browser sandbox.

## Build Commands

```bash
# Native desktop build
cargo run --release

# WebAssembly build (requires wasm32-unknown-unknown target)
# rustup target add wasm32-unknown-unknown
```

## Architecture

- **Core Logic**: Rust - handles JSON parsing, formatting, and validation
- **GUI**: Qt 6 - provides cross-platform desktop-grade interface
- **Platform**: WebAssembly - enables browser execution
- **Deployment**: Static hosting via GitHub Pages

## Key Design Constraints

- Zero external API calls after WASM binary loads
- All data processing must remain client-side (privacy requirement)
- Must handle sensitive JSON payloads (API keys, PII, credentials) without any data leaving the device

## AsyncSerialiser Usage

The `AsyncSerialiser` class serializes async operations to prevent concurrent Asyncify suspensions in WASM:

```cpp
#include "asyncserialiser.h"

// Enqueue an async task
AsyncSerialiser::instance().enqueue("taskName", []() {
    QPromise<QVariant> promise;
    auto future = promise.future();
    promise.start();

    // Perform async work (e.g., val::await)
    // ...

    promise.addResult(QVariant::fromValue(result));
    promise.finish();
    return future;
});

// Connect to signals for task lifecycle
connect(&AsyncSerialiser::instance(), &AsyncSerialiser::taskCompleted,
        this, [](const QString& name, bool success) {
    // Handle completion
});
```

Key features:
- **Single-flight execution**: Only one task runs at a time (m_isBusy guard)
- **FIFO ordering**: Tasks execute in enqueue order
- **Watchdog timer**: 30-second timeout prevents hung tasks from blocking the queue (uses emscripten_set_timeout fallback in WASM for reliability)
- **Error isolation**: Exceptions in one task don't block subsequent tasks
- **Queue bounds**: Maximum 100 tasks (emits `taskRejected` if exceeded), warning at >10 tasks (`queueLengthWarning` signal)
