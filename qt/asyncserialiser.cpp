/**
 * @file asyncserialiser.cpp
 * @brief Implementation of AsyncSerialiser task queue
 */
#include "asyncserialiser.h"
#include <QDebug>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/val.h>
using emscripten::val;
#endif

AsyncSerialiser& AsyncSerialiser::instance()
{
    static AsyncSerialiser instance;
    return instance;
}

AsyncSerialiser::AsyncSerialiser()
{
    m_watchdog.setSingleShot(true);
    m_watchdog.setInterval(WATCHDOG_TIMEOUT_MS);
    connect(&m_watchdog, &QTimer::timeout, this, &AsyncSerialiser::onWatchdogTimeout);
}

AsyncSerialiser::~AsyncSerialiser()
{
    clearQueue();
}

void AsyncSerialiser::enqueue(const QString& taskName, AsyncTask task)
{
    /*
     * FUTURE JSPI BYPASS (Experimental)
     * ==================================
     * When JSPI (JavaScript Promise Integration) is widely supported, this bypass
     * can be enabled to execute tasks directly without queuing. JSPI supports
     * multiple concurrent WebAssembly suspensions, eliminating Asyncify's
     * single-flight limitation.
     *
     * To enable JSPI bypass:
     * 1. Build with -DENABLE_JSPI=ON in CMake
     * 2. Uncomment the ENABLE_JSPI_RUNTIME_BYPASS block below
     * 3. Test thoroughly in Chrome 137+ (or Firefox 130+ with flag enabled)
     *
     * The bypass maintains the same public API and signal emission pattern,
     * ensuring drop-in compatibility when switching between Asyncify and JSPI modes.
     *
     * Browser Support Status (as of 2026-01):
     * - Chrome 137+: JSPI enabled by default
     * - Edge 137+: JSPI enabled (Chromium-based)
     * - Firefox 130+: Behind flag (javascript.options.wasm_jspi)
     * - Safari: Not supported (no timeline announced)
     *
     * Uncomment to enable:
     *
     * #ifdef ENABLE_JSPI_RUNTIME_BYPASS
     * if (jspiAvailable()) {
     *     // JSPI mode: Execute task directly without queuing
     *     // JSPI supports concurrent suspensions, so no serialization needed
     *     qDebug() << "[AsyncSerialiser] JSPI mode: direct execution for" << taskName;
     *     emit taskStarted(taskName);
     *
     *     try {
     *         QFuture<QVariant> future = task();
     *
     *         // Set up watcher for completion notification
     *         auto* watcher = new QFutureWatcher<QVariant>(this);
     *         connect(watcher, &QFutureWatcher<QVariant>::finished, this,
     *             [this, watcher, taskName]() {
     *                 bool success = !watcher->future().isCanceled();
     *                 qDebug() << "[AsyncSerialiser] JSPI task completed:" << taskName
     *                          << "Success:" << success;
     *                 emit taskCompleted(taskName, success);
     *                 watcher->deleteLater();
     *             });
     *         watcher->setFuture(future);
     *     } catch (const std::exception& e) {
     *         qWarning() << "[AsyncSerialiser] JSPI task exception:" << taskName
     *                    << "-" << e.what();
     *         emit taskCompleted(taskName, false);
     *     } catch (...) {
     *         qWarning() << "[AsyncSerialiser] JSPI task unknown exception:" << taskName;
     *         emit taskCompleted(taskName, false);
     *     }
     *     return;
     * }
     * #endif
     */

    // Standard queue-based execution (Asyncify mode)
    // This path serializes tasks to prevent concurrent Asyncify suspensions

    // Check queue size limit to prevent unbounded growth
    if (m_queue.size() >= MAX_QUEUE_SIZE) {
        qWarning() << "[AsyncSerialiser] Queue full (" << MAX_QUEUE_SIZE
                   << "), rejecting task:" << taskName;
        emit taskRejected(taskName);
        return;
    }

    m_queue.enqueue({taskName, std::move(task)});
    emit queueLengthChanged();

    qDebug() << "[AsyncSerialiser] Enqueued task:" << taskName
             << "Queue size:" << m_queue.size();

    // Emit warning if queue is getting long
    if (m_queue.size() > QUEUE_LENGTH_WARNING_THRESHOLD) {
        qWarning() << "[AsyncSerialiser] Queue length warning: " << m_queue.size()
                   << " tasks pending (threshold: " << QUEUE_LENGTH_WARNING_THRESHOLD << ")";
        emit queueLengthWarning(m_queue.size());
    }

    // Use invokeMethod to process on next event loop tick
    // This prevents reentrancy issues
    QMetaObject::invokeMethod(this, &AsyncSerialiser::processNext, Qt::QueuedConnection);
}

void AsyncSerialiser::clearQueue()
{
    qDebug() << "[AsyncSerialiser] Clearing queue. Pending tasks:" << m_queue.size();

    m_queue.clear();
    m_watchdog.stop();
#ifdef __EMSCRIPTEN__
    stopEmscriptenWatchdog();
#endif

    if (m_watcher) {
        m_watcher->cancel();
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }

    m_isBusy = false;
    emit queueLengthChanged();
}

void AsyncSerialiser::processNext()
{
    // CRITICAL: The single-flight guard
    if (m_isBusy || m_queue.isEmpty()) {
        return;
    }

    m_isBusy = true;
    QueuedTask queued = m_queue.dequeue();
    m_currentTaskName = queued.name;
    emit queueLengthChanged();

    qDebug() << "[AsyncSerialiser] Starting task:" << m_currentTaskName
             << "Queue remaining:" << m_queue.size();
    emit taskStarted(m_currentTaskName);

    // Start watchdog (Qt timer + emscripten fallback for WASM reliability)
    m_watchdog.start();
#ifdef __EMSCRIPTEN__
    startEmscriptenWatchdog();
#endif

    // Execute the task - handle exceptions to prevent queue blockage
    QFuture<QVariant> future;
    try {
        // Execute the task - this may trigger Asyncify suspension
        future = queued.task();
    } catch (const std::exception& e) {
        qWarning() << "[AsyncSerialiser] Exception in task" << m_currentTaskName
                   << ":" << e.what();
        m_watchdog.stop();
        emit taskCompleted(m_currentTaskName, false);
        m_isBusy = false;
        processNext();
        return;
    } catch (...) {
        qWarning() << "[AsyncSerialiser] Unknown exception in task" << m_currentTaskName;
        m_watchdog.stop();
        emit taskCompleted(m_currentTaskName, false);
        m_isBusy = false;
        processNext();
        return;
    }

    // Set up watcher for completion
    m_watcher = new QFutureWatcher<QVariant>(this);
    connect(m_watcher, &QFutureWatcher<QVariant>::finished,
            this, &AsyncSerialiser::onTaskFinished);
    m_watcher->setFuture(future);
}

void AsyncSerialiser::onTaskFinished()
{
    m_watchdog.stop();
#ifdef __EMSCRIPTEN__
    stopEmscriptenWatchdog();
#endif

    bool success = !m_watcher->future().isCanceled();
    qDebug() << "[AsyncSerialiser] Task completed:" << m_currentTaskName
             << "Success:" << success;

    emit taskCompleted(m_currentTaskName, success);

    m_watcher->deleteLater();
    m_watcher = nullptr;
    m_isBusy = false;

    // Chain to next task
    processNext();
}

void AsyncSerialiser::onWatchdogTimeout()
{
    qWarning() << "[AsyncSerialiser] WATCHDOG TIMEOUT for task:" << m_currentTaskName;

#ifdef __EMSCRIPTEN__
    stopEmscriptenWatchdog();
#endif

    emit taskTimedOut(m_currentTaskName);
    emit taskCompleted(m_currentTaskName, false);

    if (m_watcher) {
        m_watcher->cancel();
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }

    m_isBusy = false;
    processNext();
}

#ifdef __EMSCRIPTEN__
void AsyncSerialiser::startEmscriptenWatchdog()
{
    stopEmscriptenWatchdog(); // Clear any existing timer
    m_emscriptenTimerId = emscripten_set_timeout(
        &AsyncSerialiser::emscriptenWatchdogCallback,
        WATCHDOG_TIMEOUT_MS,
        this
    );
    qDebug() << "[AsyncSerialiser] Started emscripten watchdog timer id:" << m_emscriptenTimerId;
}

void AsyncSerialiser::stopEmscriptenWatchdog()
{
    if (m_emscriptenTimerId != 0) {
        emscripten_clear_timeout(m_emscriptenTimerId);
        m_emscriptenTimerId = 0;
    }
}

void AsyncSerialiser::emscriptenWatchdogCallback(void* userData)
{
    auto* self = static_cast<AsyncSerialiser*>(userData);
    self->m_emscriptenTimerId = 0; // Timer has fired, clear the ID
    // Invoke through Qt event loop for thread safety
    QMetaObject::invokeMethod(self, &AsyncSerialiser::onWatchdogTimeout, Qt::QueuedConnection);
}
#endif

bool AsyncSerialiser::jspiAvailable()
{
#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        if (window.hasOwnProperty("JSPI_AVAILABLE")) {
            return window["JSPI_AVAILABLE"].as<bool>();
        }
    } catch (...) {
        // If JavaScript access fails, fall back to false
        qDebug() << "[AsyncSerialiser] JSPI detection failed, defaulting to Asyncify mode";
    }
#endif
    return false;
}
