#ifndef JSONBRIDGE_H
#define JSONBRIDGE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include "qjsontreemodel.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>
#endif

class JsonBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(QJsonTreeModel* treeModel READ treeModel CONSTANT)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit JsonBridge(QObject *parent = nullptr);

    bool isReady() const;
    QJsonTreeModel* treeModel() const;
    bool isBusy() const;

    // Async operations (fire-and-forget, results via signals)
    Q_INVOKABLE void formatJson(const QString &input, const QString &indentType);
    Q_INVOKABLE void minifyJson(const QString &input);
    Q_INVOKABLE void validateJson(const QString &input);

    // Synchronous operations
    Q_INVOKABLE QString highlightJson(const QString &input);
    Q_INVOKABLE bool loadTreeModel(const QString &json);

    // Async clipboard operations (results via signals)
    Q_INVOKABLE void copyToClipboard(const QString &text);
    Q_INVOKABLE void readFromClipboard();

    // Async history methods (results via signals)
    Q_INVOKABLE void saveToHistory(const QString &json);
    Q_INVOKABLE void loadHistory();
    Q_INVOKABLE void getHistoryEntry(const QString &id);
    Q_INVOKABLE void deleteHistoryEntry(const QString &id);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE bool isHistoryAvailable();

signals:
    // Format operations
    void formatCompleted(const QVariantMap &result);
    void minifyCompleted(const QVariantMap &result);
    void validateCompleted(const QVariantMap &result);

    // History operations
    void historySaved(bool success, const QString &id);
    void historyLoaded(const QVariantList &entries);
    void historyEntryLoaded(const QString &content);
    void historyEntryDeleted(bool success);
    void historyCleared(bool success);

    // Clipboard operations
    void copyCompleted(bool success);
    void clipboardRead(const QString &content);

    // State signals
    void readyChanged();
    void busyChanged(bool busy);

private:
    bool m_ready = false;
    QJsonTreeModel* m_treeModel;
    void checkReady();
    void connectAsyncSerialiserSignals();
};

#endif // JSONBRIDGE_H
