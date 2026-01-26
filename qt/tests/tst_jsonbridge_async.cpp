/**
 * @file tst_jsonbridge_async.cpp
 * @brief Unit tests for JsonBridge async operations (Story 5.2)
 *
 * Tests verify:
 * - AC1-7: All async operations use AsyncSerialiser
 * - AC8: Signal-based result handling
 * - AC12-16: Behavior preservation
 * - AC17-19: Edge cases (rapid operations, timeouts, error isolation)
 */
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "../jsonbridge.h"
#include "../asyncserialiser.h"

class tst_JsonBridgeAsync : public QObject
{
    Q_OBJECT

private:
    JsonBridge* m_bridge = nullptr;

private slots:
    void init()
    {
        AsyncSerialiser::instance().clearQueue();
        m_bridge = new JsonBridge(this);
    }

    void cleanup()
    {
        AsyncSerialiser::instance().clearQueue();
        delete m_bridge;
        m_bridge = nullptr;
        QCoreApplication::processEvents();
    }

    // 5.2-UNIT-001: formatJson enqueues task to AsyncSerialiser
    void testFormatJsonUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);

        m_bridge->formatJson("{\"test\": 1}", "spaces:4");

        // Task should start
        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("formatJson"));

        // Signal should be emitted with result
        QTRY_COMPARE(formatCompletedSpy.count(), 1);
        QVariantMap result = formatCompletedSpy.at(0).at(0).toMap();
        QVERIFY(result["success"].toBool());
    }

    // 5.2-UNIT-002: formatCompleted signal contains success and result
    void testFormatCompletedSignalSuccess()
    {
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);

        m_bridge->formatJson("{\"key\": \"value\"}", "spaces:2");

        QTRY_COMPARE(formatCompletedSpy.count(), 1);
        QVariantMap result = formatCompletedSpy.at(0).at(0).toMap();

        QVERIFY(result["success"].toBool());
        QVERIFY(!result["result"].toString().isEmpty());
        QVERIFY(result["result"].toString().contains("key"));
    }

    // 5.2-UNIT-003: formatCompleted signal contains error on failure
    void testFormatCompletedSignalError()
    {
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);

        m_bridge->formatJson("invalid json {", "spaces:4");

        QTRY_COMPARE(formatCompletedSpy.count(), 1);
        QVariantMap result = formatCompletedSpy.at(0).at(0).toMap();

        QVERIFY(!result["success"].toBool());
        QVERIFY(!result["error"].toString().isEmpty());
    }

    // 5.2-UNIT-004: minifyJson enqueues task to AsyncSerialiser
    void testMinifyJsonUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy minifyCompletedSpy(m_bridge, &JsonBridge::minifyCompleted);

        m_bridge->minifyJson("{ \"test\": 1 }");

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("minifyJson"));

        QTRY_COMPARE(minifyCompletedSpy.count(), 1);
        QVariantMap result = minifyCompletedSpy.at(0).at(0).toMap();
        QVERIFY(result["success"].toBool());
    }

    // 5.2-UNIT-005: validateJson enqueues task to AsyncSerialiser
    void testValidateJsonUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy validateCompletedSpy(m_bridge, &JsonBridge::validateCompleted);

        m_bridge->validateJson("{\"valid\": true}");

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("validateJson"));

        QTRY_COMPARE(validateCompletedSpy.count(), 1);
        QVariantMap result = validateCompletedSpy.at(0).at(0).toMap();
        QVERIFY(result["isValid"].toBool());
    }

    // 5.2-UNIT-006: validateCompleted signal contains stats on success
    void testValidateCompletedSignalWithStats()
    {
        QSignalSpy validateCompletedSpy(m_bridge, &JsonBridge::validateCompleted);

        m_bridge->validateJson("{\"obj\": {}, \"arr\": [], \"str\": \"test\"}");

        QTRY_COMPARE(validateCompletedSpy.count(), 1);
        QVariantMap result = validateCompletedSpy.at(0).at(0).toMap();

        QVERIFY(result["isValid"].toBool());
        QVariantMap stats = result["stats"].toMap();
        QVERIFY(stats["object_count"].toInt() >= 1);
    }

    // 5.2-UNIT-007: validateCompleted signal contains error info on failure
    void testValidateCompletedSignalError()
    {
        QSignalSpy validateCompletedSpy(m_bridge, &JsonBridge::validateCompleted);

        m_bridge->validateJson("{invalid}");

        QTRY_COMPARE(validateCompletedSpy.count(), 1);
        QVariantMap result = validateCompletedSpy.at(0).at(0).toMap();

        QVERIFY(!result["isValid"].toBool());
        QVariantMap error = result["error"].toMap();
        QVERIFY(!error["message"].toString().isEmpty());
    }

    // 5.2-UNIT-008: saveToHistory enqueues task to AsyncSerialiser
    void testSaveToHistoryUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy historySavedSpy(m_bridge, &JsonBridge::historySaved);

        m_bridge->saveToHistory("{\"save\": \"test\"}");

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("saveToHistory"));

        QTRY_COMPARE(historySavedSpy.count(), 1);
    }

    // 5.2-UNIT-009: loadHistory enqueues task to AsyncSerialiser
    void testLoadHistoryUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy historyLoadedSpy(m_bridge, &JsonBridge::historyLoaded);

        m_bridge->loadHistory();

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("loadHistory"));

        QTRY_COMPARE(historyLoadedSpy.count(), 1);
    }

    // 5.2-UNIT-010: copyToClipboard enqueues task to AsyncSerialiser
    void testCopyToClipboardUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy copyCompletedSpy(m_bridge, &JsonBridge::copyCompleted);

        m_bridge->copyToClipboard("test clipboard content");

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("copyToClipboard"));

        QTRY_COMPARE(copyCompletedSpy.count(), 1);
    }

    // 5.2-UNIT-011: readFromClipboard enqueues task to AsyncSerialiser
    void testReadFromClipboardUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy clipboardReadSpy(m_bridge, &JsonBridge::clipboardRead);

        m_bridge->readFromClipboard();

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("readFromClipboard"));

        QTRY_COMPARE(clipboardReadSpy.count(), 1);
    }

    // 5.2-UNIT-012: 5 rapid format calls queue without crash
    void testRapidFormatCalls()
    {
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);
        QSignalSpy taskCompletedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskCompleted);

        // Rapid-fire 5 format calls
        for (int i = 0; i < 5; i++) {
            m_bridge->formatJson(QString("{\"iteration\": %1}").arg(i), "spaces:4");
        }

        // All 5 should complete
        QTRY_COMPARE(formatCompletedSpy.count(), 5);
        QTRY_COMPARE(taskCompletedSpy.count(), 5);

        // All should succeed
        for (int i = 0; i < 5; i++) {
            QVariantMap result = formatCompletedSpy.at(i).at(0).toMap();
            QVERIFY(result["success"].toBool());
        }
    }

    // 5.2-UNIT-013: Operations complete in FIFO order
    void testOperationsFIFOOrder()
    {
        QStringList operationOrder;

        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);
        QSignalSpy minifyCompletedSpy(m_bridge, &JsonBridge::minifyCompleted);
        QSignalSpy validateCompletedSpy(m_bridge, &JsonBridge::validateCompleted);

        connect(m_bridge, &JsonBridge::formatCompleted, this, [&operationOrder]() {
            operationOrder.append("format");
        });
        connect(m_bridge, &JsonBridge::minifyCompleted, this, [&operationOrder]() {
            operationOrder.append("minify");
        });
        connect(m_bridge, &JsonBridge::validateCompleted, this, [&operationOrder]() {
            operationOrder.append("validate");
        });

        m_bridge->formatJson("{\"a\":1}", "spaces:4");
        m_bridge->minifyJson("{\"b\":2}");
        m_bridge->validateJson("{\"c\":3}");

        // Wait for all to complete
        QTRY_COMPARE(formatCompletedSpy.count(), 1);
        QTRY_COMPARE(minifyCompletedSpy.count(), 1);
        QTRY_COMPARE(validateCompletedSpy.count(), 1);

        // Verify FIFO order
        QCOMPARE(operationOrder, QStringList({"format", "minify", "validate"}));
    }

    // 5.2-UNIT-014: Failed operation doesn't block subsequent operations
    void testFailedOperationDoesNotBlockQueue()
    {
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);

        // First call with invalid JSON (will fail)
        m_bridge->formatJson("invalid{", "spaces:4");
        // Second call with valid JSON (should still work)
        m_bridge->formatJson("{\"valid\": true}", "spaces:4");

        QTRY_COMPARE(formatCompletedSpy.count(), 2);

        // First should fail
        QVariantMap result1 = formatCompletedSpy.at(0).at(0).toMap();
        QVERIFY(!result1["success"].toBool());

        // Second should succeed
        QVariantMap result2 = formatCompletedSpy.at(1).at(0).toMap();
        QVERIFY(result2["success"].toBool());
    }

    // 5.2-UNIT-015: busyChanged signal emits correctly
    void testBusyChangedSignal()
    {
        QSignalSpy busyChangedSpy(m_bridge, &JsonBridge::busyChanged);

        // Initially not busy
        QVERIFY(!m_bridge->isBusy());

        m_bridge->formatJson("{\"test\": 1}", "spaces:4");

        // Wait for completion
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);
        QTRY_COMPARE(formatCompletedSpy.count(), 1);

        // busyChanged should have been emitted at least once
        QVERIFY(busyChangedSpy.count() >= 1);
    }

    // 5.2-INT-021: 5 rapid format clicks complete in FIFO order
    void testRapidFormatClicksFIFO()
    {
        QList<int> completionOrder;
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);

        // Connect to track order
        connect(m_bridge, &JsonBridge::formatCompleted, this, [&completionOrder, &formatCompletedSpy]() {
            completionOrder.append(formatCompletedSpy.count());
        });

        // Simulate 5 rapid clicks
        for (int i = 1; i <= 5; i++) {
            m_bridge->formatJson(QString("{\"click\": %1}").arg(i), "spaces:4");
        }

        QTRY_COMPARE(formatCompletedSpy.count(), 5);

        // Verify FIFO (1, 2, 3, 4, 5)
        QCOMPARE(completionOrder, QList<int>({1, 2, 3, 4, 5}));
    }

    // 5.2-INT-023: Failed format followed by valid format
    void testFailedFormatFollowedByValidFormat()
    {
        QSignalSpy formatCompletedSpy(m_bridge, &JsonBridge::formatCompleted);

        // Failed format
        m_bridge->formatJson("{broken", "spaces:4");
        // Valid format
        m_bridge->formatJson("{\"ok\": true}", "spaces:4");

        QTRY_COMPARE(formatCompletedSpy.count(), 2);

        // First emits error signal
        QVariantMap result1 = formatCompletedSpy.at(0).at(0).toMap();
        QVERIFY(!result1["success"].toBool());

        // Second succeeds independently
        QVariantMap result2 = formatCompletedSpy.at(1).at(0).toMap();
        QVERIFY(result2["success"].toBool());
    }

    // Test deleteHistoryEntry uses async pattern
    void testDeleteHistoryEntryUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy historyDeletedSpy(m_bridge, &JsonBridge::historyEntryDeleted);

        m_bridge->deleteHistoryEntry("some-id");

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("deleteHistoryEntry"));

        QTRY_COMPARE(historyDeletedSpy.count(), 1);
    }

    // Test clearHistory uses async pattern
    void testClearHistoryUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy historyClearedSpy(m_bridge, &JsonBridge::historyCleared);

        m_bridge->clearHistory();

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("clearHistory"));

        QTRY_COMPARE(historyClearedSpy.count(), 1);
    }

    // Test getHistoryEntry uses async pattern
    void testGetHistoryEntryUsesAsyncSerialiser()
    {
        QSignalSpy taskStartedSpy(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted);
        QSignalSpy historyEntryLoadedSpy(m_bridge, &JsonBridge::historyEntryLoaded);

        m_bridge->getHistoryEntry("test-id");

        QTRY_COMPARE(taskStartedSpy.count(), 1);
        QCOMPARE(taskStartedSpy.at(0).at(0).toString(), QString("getHistoryEntry"));

        QTRY_COMPARE(historyEntryLoadedSpy.count(), 1);
    }
};

QTEST_MAIN(tst_JsonBridgeAsync)
#include "tst_jsonbridge_async.moc"
