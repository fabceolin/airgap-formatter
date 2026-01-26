#include "jsonbridge.h"
#include "asyncserialiser.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QGuiApplication>
#include <QClipboard>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QUuid>
#include <QPromise>

#ifdef __EMSCRIPTEN__
#include <emscripten/val.h>
using namespace emscripten;
#endif

// Desktop-only: JSON formatting with indentation
#ifndef __EMSCRIPTEN__
static QString formatJsonNative(const QString &input, const QString &indentType) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(input.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        return QString();
    }

    QJsonDocument::JsonFormat format = QJsonDocument::Indented;
    return QString::fromUtf8(doc.toJson(format));
}

static QString minifyJsonNative(const QString &input) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(input.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        return QString();
    }

    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

static void countJsonStats(const QJsonValue &value, QVariantMap &stats, int depth) {
    int maxDepth = stats["max_depth"].toInt();
    if (depth > maxDepth) {
        stats["max_depth"] = depth;
    }

    if (value.isObject()) {
        stats["object_count"] = stats["object_count"].toInt() + 1;
        QJsonObject obj = value.toObject();
        stats["total_keys"] = stats["total_keys"].toInt() + obj.keys().size();
        for (const QString &key : obj.keys()) {
            countJsonStats(obj[key], stats, depth + 1);
        }
    } else if (value.isArray()) {
        stats["array_count"] = stats["array_count"].toInt() + 1;
        QJsonArray arr = value.toArray();
        for (const QJsonValue &v : arr) {
            countJsonStats(v, stats, depth + 1);
        }
    } else if (value.isString()) {
        stats["string_count"] = stats["string_count"].toInt() + 1;
    } else if (value.isDouble()) {
        stats["number_count"] = stats["number_count"].toInt() + 1;
    } else if (value.isBool()) {
        stats["boolean_count"] = stats["boolean_count"].toInt() + 1;
    } else if (value.isNull()) {
        stats["null_count"] = stats["null_count"].toInt() + 1;
    }
}

static QString getHistoryFilePath() {
    // Check if running in Docker/container (workspace directory exists)
    QDir workspaceDir("/workspace");
    if (workspaceDir.exists()) {
        // Use workspace directory for persistence in Docker
        return "/workspace/.history.json";
    }

    // Use standard app data location for native desktop
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dataPath + "/history.json";
}

static QJsonArray loadHistoryFromFile() {
    QFile file(getHistoryFilePath());
    if (!file.exists()) {
        return QJsonArray();
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonArray();
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
        return doc.array();
    }
    return QJsonArray();
}

static bool saveHistoryToFile(const QJsonArray &history) {
    QFile file(getHistoryFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QJsonDocument doc(history);
    file.write(doc.toJson());
    file.close();
    return true;
}

static QString highlightJsonNative(const QString &input) {
    // Syntax highlighting for desktop - produces jq-like colored output
    // Wrap in <pre> to preserve whitespace and newlines
    QString result = "<pre style=\"margin:0; font-family:monospace; white-space:pre-wrap;\">";
    bool inString = false;
    bool inKey = false;
    bool escapeNext = false;

    for (int i = 0; i < input.length(); ++i) {
        QChar c = input[i];

        if (escapeNext) {
            // Handle escaped character - escape HTML entities if needed
            if (c == '<') result += "&lt;";
            else if (c == '>') result += "&gt;";
            else if (c == '&') result += "&amp;";
            else result += c;
            escapeNext = false;
            continue;
        }

        if (c == '\\' && inString) {
            result += c;
            escapeNext = true;
            continue;
        }

        if (c == '"') {
            if (!inString) {
                // Check if this is a key (followed eventually by :)
                int j = i + 1;
                while (j < input.length() && input[j] != '"') {
                    if (input[j] == '\\') j++;
                    j++;
                }
                j++; // skip closing quote
                while (j < input.length() && input[j].isSpace()) j++;
                inKey = (j < input.length() && input[j] == ':');

                if (inKey) {
                    result += "<span style=\"color:#8fa1b3;\">\"";  // Keys: light blue (jq style)
                } else {
                    result += "<span style=\"color:#a3be8c;\">\"";  // Strings: green (jq style)
                }
                inString = true;
            } else {
                result += "\"</span>";
                inString = false;
                inKey = false;
            }
            continue;
        }

        if (inString) {
            // Escape HTML entities
            if (c == '<') result += "&lt;";
            else if (c == '>') result += "&gt;";
            else if (c == '&') result += "&amp;";
            else result += c;
            continue;
        }

        // Numbers
        if (c.isDigit() || (c == '-' && i + 1 < input.length() && input[i+1].isDigit())) {
            result += "<span style=\"color:#d08770;\">";  // Numbers: orange (jq style)
            while (i < input.length() && (input[i].isDigit() || input[i] == '.' || input[i] == '-' || input[i] == 'e' || input[i] == 'E' || input[i] == '+')) {
                result += input[i];
                i++;
            }
            result += "</span>";
            i--;
            continue;
        }

        // true/false
        if (input.mid(i, 4) == "true") {
            result += "<span style=\"color:#b48ead;\">true</span>";  // Booleans: purple (jq style)
            i += 3;
            continue;
        }
        if (input.mid(i, 5) == "false") {
            result += "<span style=\"color:#b48ead;\">false</span>";  // Booleans: purple
            i += 4;
            continue;
        }
        // null
        if (input.mid(i, 4) == "null") {
            result += "<span style=\"color:#bf616a;\">null</span>";  // Null: red (jq style)
            i += 3;
            continue;
        }

        // Brackets and braces
        if (c == '{' || c == '}' || c == '[' || c == ']') {
            result += "<span style=\"color:#c0c5ce;\">" + QString(c) + "</span>";  // Punctuation: light gray
            continue;
        }

        // Colon and comma
        if (c == ':' || c == ',') {
            result += "<span style=\"color:#c0c5ce;\">" + QString(c) + "</span>";  // Punctuation: light gray
            continue;
        }

        // All other characters (whitespace, newlines) - pass through
        result += c;
    }

    result += "</pre>";
    return result;
}
#endif

JsonBridge::JsonBridge(QObject *parent)
    : QObject(parent)
    , m_treeModel(new QJsonTreeModel(this))
{
    checkReady();
    connectAsyncSerialiserSignals();
}

void JsonBridge::connectAsyncSerialiserSignals()
{
    // Connect to AsyncSerialiser to emit busyChanged when queue changes
    connect(&AsyncSerialiser::instance(), &AsyncSerialiser::queueLengthChanged,
            this, [this]() {
        emit busyChanged(isBusy());
    });
    connect(&AsyncSerialiser::instance(), &AsyncSerialiser::taskStarted,
            this, [this](const QString&) {
        emit busyChanged(isBusy());
    });
    connect(&AsyncSerialiser::instance(), &AsyncSerialiser::taskCompleted,
            this, [this](const QString&, bool) {
        emit busyChanged(isBusy());
    });
}

bool JsonBridge::isBusy() const
{
    return AsyncSerialiser::instance().queueLength() > 0;
}

QJsonTreeModel* JsonBridge::treeModel() const
{
    return m_treeModel;
}

bool JsonBridge::loadTreeModel(const QString &json)
{
    return m_treeModel->loadJson(json);
}

void JsonBridge::checkReady()
{
#ifdef __EMSCRIPTEN__
    val window = val::global("window");
    val jsonBridge = window["JsonBridge"];
    if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
        val isReadyFunc = jsonBridge["isReady"];
        if (!isReadyFunc.isUndefined()) {
            m_ready = jsonBridge.call<bool>("isReady");
        }
    }
#else
    // Desktop mode is always ready
    m_ready = true;
#endif
    emit readyChanged();
}

bool JsonBridge::isReady() const
{
    return m_ready;
}

void JsonBridge::formatJson(const QString &input, const QString &indentType)
{
    AsyncSerialiser::instance().enqueue("formatJson", [this, input, indentType]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantMap result;
        result["success"] = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                result["error"] = "JsonBridge not available";
            } else {
                std::string inputStd = input.toStdString();
                std::string indentStd = indentType.toStdString();

                val jsResult = jsonBridge.call<val>("formatJson", inputStd, indentStd);

                bool success = jsResult["success"].as<bool>();
                result["success"] = success;

                if (success) {
                    std::string resultStr = jsResult["result"].as<std::string>();
                    result["result"] = QString::fromStdString(resultStr);
                } else {
                    std::string errorStr = jsResult["error"].as<std::string>();
                    result["error"] = QString::fromStdString(errorStr);
                }
            }
        } catch (const std::exception &e) {
            result["error"] = QString("Exception: %1").arg(e.what());
        } catch (...) {
            result["error"] = "Unknown error in formatJson";
        }
#else
        // Desktop native implementation
        QString formatted = formatJsonNative(input, indentType);
        if (formatted.isEmpty()) {
            result["error"] = "Invalid JSON";
        } else {
            result["success"] = true;
            result["result"] = formatted;
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, result]() {
            emit formatCompleted(result);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(result));
        promise.finish();
        return future;
    });
}

void JsonBridge::minifyJson(const QString &input)
{
    AsyncSerialiser::instance().enqueue("minifyJson", [this, input]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantMap result;
        result["success"] = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                result["error"] = "JsonBridge not available";
            } else {
                std::string inputStd = input.toStdString();

                val jsResult = jsonBridge.call<val>("minifyJson", inputStd);

                bool success = jsResult["success"].as<bool>();
                result["success"] = success;

                if (success) {
                    std::string resultStr = jsResult["result"].as<std::string>();
                    result["result"] = QString::fromStdString(resultStr);
                } else {
                    std::string errorStr = jsResult["error"].as<std::string>();
                    result["error"] = QString::fromStdString(errorStr);
                }
            }
        } catch (const std::exception &e) {
            result["error"] = QString("Exception: %1").arg(e.what());
        } catch (...) {
            result["error"] = "Unknown error in minifyJson";
        }
#else
        // Desktop native implementation
        QString minified = minifyJsonNative(input);
        if (minified.isEmpty()) {
            result["error"] = "Invalid JSON";
        } else {
            result["success"] = true;
            result["result"] = minified;
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, result]() {
            emit minifyCompleted(result);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(result));
        promise.finish();
        return future;
    });
}

void JsonBridge::validateJson(const QString &input)
{
    AsyncSerialiser::instance().enqueue("validateJson", [this, input]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantMap result;
        result["isValid"] = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
                QVariantMap error;
                error["message"] = "JsonBridge not available";
                error["line"] = 0;
                error["column"] = 0;
                result["error"] = error;
                result["stats"] = QVariantMap();
            } else {
                std::string inputStd = input.toStdString();

                val jsResult = jsonBridge.call<val>("validateJson", inputStd);

                bool isValid = jsResult["isValid"].as<bool>();
                result["isValid"] = isValid;

                if (isValid) {
                    QVariantMap stats;
                    val jsStats = jsResult["stats"];
                    if (!jsStats.isUndefined()) {
                        stats["object_count"] = jsStats["object_count"].isUndefined() ? 0 : jsStats["object_count"].as<int>();
                        stats["array_count"] = jsStats["array_count"].isUndefined() ? 0 : jsStats["array_count"].as<int>();
                        stats["string_count"] = jsStats["string_count"].isUndefined() ? 0 : jsStats["string_count"].as<int>();
                        stats["number_count"] = jsStats["number_count"].isUndefined() ? 0 : jsStats["number_count"].as<int>();
                        stats["boolean_count"] = jsStats["boolean_count"].isUndefined() ? 0 : jsStats["boolean_count"].as<int>();
                        stats["null_count"] = jsStats["null_count"].isUndefined() ? 0 : jsStats["null_count"].as<int>();
                        stats["total_keys"] = jsStats["total_keys"].isUndefined() ? 0 : jsStats["total_keys"].as<int>();
                        stats["max_depth"] = jsStats["max_depth"].isUndefined() ? 0 : jsStats["max_depth"].as<int>();
                    }
                    result["stats"] = stats;
                } else {
                    val jsError = jsResult["error"];
                    QVariantMap error;
                    error["message"] = jsError["message"].isUndefined() ? QString("Unknown error") : QString::fromStdString(jsError["message"].as<std::string>());
                    error["line"] = jsError["line"].isUndefined() ? 0 : jsError["line"].as<int>();
                    error["column"] = jsError["column"].isUndefined() ? 0 : jsError["column"].as<int>();
                    result["error"] = error;
                    result["stats"] = QVariantMap();
                }
            }
        } catch (const std::exception &e) {
            QVariantMap error;
            error["message"] = QString("Exception: %1").arg(e.what());
            error["line"] = 0;
            error["column"] = 0;
            result["error"] = error;
            result["stats"] = QVariantMap();
        } catch (...) {
            QVariantMap error;
            error["message"] = "Unknown error in validateJson";
            error["line"] = 0;
            error["column"] = 0;
            result["error"] = error;
            result["stats"] = QVariantMap();
        }
#else
        // Desktop native implementation
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(input.toUtf8(), &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            QVariantMap error;
            error["message"] = parseError.errorString();
            // Calculate line and column from offset
            int line = 1, column = 1;
            for (int i = 0; i < parseError.offset && i < input.length(); ++i) {
                if (input[i] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
            }
            error["line"] = line;
            error["column"] = column;
            result["error"] = error;
            result["stats"] = QVariantMap();
        } else {
            result["isValid"] = true;
            QVariantMap stats;
            stats["object_count"] = 0;
            stats["array_count"] = 0;
            stats["string_count"] = 0;
            stats["number_count"] = 0;
            stats["boolean_count"] = 0;
            stats["null_count"] = 0;
            stats["total_keys"] = 0;
            stats["max_depth"] = 0;

            if (doc.isObject()) {
                countJsonStats(doc.object(), stats, 1);
            } else if (doc.isArray()) {
                countJsonStats(doc.array(), stats, 1);
            }
            result["stats"] = stats;
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, result]() {
            emit validateCompleted(result);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(result));
        promise.finish();
        return future;
    });
}

QString JsonBridge::highlightJson(const QString &input)
{
#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        val jsonBridge = window["JsonBridge"];

        if (jsonBridge.isUndefined() || jsonBridge.isNull()) {
            // Return escaped HTML if bridge not available
            QString escaped = input;
            escaped.replace("&", "&amp;");
            escaped.replace("<", "&lt;");
            escaped.replace(">", "&gt;");
            return escaped;
        }

        std::string inputStd = input.toStdString();
        std::string result = jsonBridge.call<std::string>("highlightJson", inputStd);
        return QString::fromStdString(result);
    } catch (const std::exception &e) {
        qWarning() << "highlightJson error:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error in highlightJson";
    }
    // Fallback: return escaped HTML
    QString escaped = input;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    return escaped;
#else
    // Desktop native implementation
    return highlightJsonNative(input);
#endif
}

void JsonBridge::copyToClipboard(const QString &text)
{
    AsyncSerialiser::instance().enqueue("copyToClipboard", [this, text]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        bool success = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                std::string textStd = text.toStdString();
                jsonBridge.call<void>("copyToClipboard", textStd);
                success = true;
            }
        } catch (...) {
            qWarning() << "Failed to copy to clipboard";
        }
#else
        // Desktop native implementation
        QClipboard *clipboard = QGuiApplication::clipboard();
        if (clipboard) {
            clipboard->setText(text);
            success = true;
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            emit copyCompleted(success);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

void JsonBridge::readFromClipboard()
{
    AsyncSerialiser::instance().enqueue("readFromClipboard", [this]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QString content;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                // Note: This is a synchronous call to an async JS function
                // The JS side handles the promise internally
                val jsPromise = jsonBridge.call<val>("readFromClipboard");
                val result = jsPromise.await();
                if (!result.isUndefined() && !result.isNull()) {
                    std::string text = result.as<std::string>();
                    content = QString::fromStdString(text);
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to read from clipboard:" << e.what();
        } catch (...) {
            qWarning() << "Failed to read from clipboard";
        }
#else
        // Desktop native implementation
        QClipboard *clipboard = QGuiApplication::clipboard();
        if (clipboard) {
            content = clipboard->text();
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, content]() {
            emit clipboardRead(content);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(content));
        promise.finish();
        return future;
    });
}

// History methods

void JsonBridge::saveToHistory(const QString &json)
{
    AsyncSerialiser::instance().enqueue("saveToHistory", [this, json]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        bool success = false;
        QString id;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                std::string jsonStd = json.toStdString();
                val jsPromise = jsonBridge.call<val>("saveToHistory", jsonStd);
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();
                    success = obj["success"].toBool();
                    id = obj["id"].toString();
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to save to history:" << e.what();
        } catch (...) {
            qWarning() << "Failed to save to history";
        }
#else
        // Desktop native implementation
        QJsonArray history = loadHistoryFromFile();

        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        QString preview = json.left(100).simplified();
        if (json.length() > 100) preview += "...";

        QJsonObject entry;
        entry["id"] = id;
        entry["content"] = json;
        entry["timestamp"] = timestamp;
        entry["preview"] = preview;
        entry["size"] = json.size();

        // Add to beginning of array (most recent first)
        QJsonArray newHistory;
        newHistory.append(entry);
        for (int i = 0; i < history.size() && i < 49; ++i) { // Keep max 50 entries
            newHistory.append(history[i]);
        }

        success = saveHistoryToFile(newHistory);
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success, id]() {
            emit historySaved(success, id);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

void JsonBridge::loadHistory()
{
    AsyncSerialiser::instance().enqueue("loadHistory", [this]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QVariantList entries;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                val jsPromise = jsonBridge.call<val>("loadHistory");
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();

                    if (obj["success"].toBool()) {
                        QJsonArray entriesArray = obj["entries"].toArray();
                        for (const QJsonValue &v : entriesArray) {
                            QJsonObject entry = v.toObject();
                            QVariantMap entryMap;
                            entryMap["id"] = entry["id"].toString();
                            entryMap["content"] = entry["content"].toString();
                            entryMap["timestamp"] = entry["timestamp"].toString();
                            entryMap["preview"] = entry["preview"].toString();
                            entryMap["size"] = entry["size"].toInt();
                            entries.append(entryMap);
                        }
                    }
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to load history:" << e.what();
        } catch (...) {
            qWarning() << "Failed to load history";
        }
#else
        // Desktop native implementation
        QJsonArray history = loadHistoryFromFile();
        for (const QJsonValue &v : history) {
            QJsonObject entry = v.toObject();
            QVariantMap entryMap;
            entryMap["id"] = entry["id"].toString();
            entryMap["content"] = entry["content"].toString();
            entryMap["timestamp"] = entry["timestamp"].toString();
            entryMap["preview"] = entry["preview"].toString();
            entryMap["size"] = entry["size"].toInt();
            entries.append(entryMap);
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, entries]() {
            emit historyLoaded(entries);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(entries));
        promise.finish();
        return future;
    });
}

void JsonBridge::getHistoryEntry(const QString &id)
{
    AsyncSerialiser::instance().enqueue("getHistoryEntry", [this, id]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        QString content;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                std::string idStd = id.toStdString();
                val jsPromise = jsonBridge.call<val>("getHistoryEntry", idStd);
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();

                    if (obj["success"].toBool()) {
                        QJsonObject entry = obj["entry"].toObject();
                        content = entry["content"].toString();
                    }
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to get history entry:" << e.what();
        } catch (...) {
            qWarning() << "Failed to get history entry";
        }
#else
        // Desktop native implementation
        QJsonArray history = loadHistoryFromFile();
        for (const QJsonValue &v : history) {
            QJsonObject entry = v.toObject();
            if (entry["id"].toString() == id) {
                content = entry["content"].toString();
                break;
            }
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, content]() {
            emit historyEntryLoaded(content);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(content));
        promise.finish();
        return future;
    });
}

void JsonBridge::deleteHistoryEntry(const QString &id)
{
    AsyncSerialiser::instance().enqueue("deleteHistoryEntry", [this, id]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        bool success = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                std::string idStd = id.toStdString();
                val jsPromise = jsonBridge.call<val>("deleteHistoryEntry", idStd);
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();
                    success = obj["success"].toBool();
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to delete history entry:" << e.what();
        } catch (...) {
            qWarning() << "Failed to delete history entry";
        }
#else
        // Desktop native implementation
        QJsonArray history = loadHistoryFromFile();
        QJsonArray newHistory;
        bool found = false;
        for (const QJsonValue &v : history) {
            QJsonObject entry = v.toObject();
            if (entry["id"].toString() != id) {
                newHistory.append(entry);
            } else {
                found = true;
            }
        }
        if (found) {
            success = saveHistoryToFile(newHistory);
        }
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            emit historyEntryDeleted(success);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

void JsonBridge::clearHistory()
{
    AsyncSerialiser::instance().enqueue("clearHistory", [this]() {
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.start();

        bool success = false;

#ifdef __EMSCRIPTEN__
        try {
            val window = val::global("window");
            val jsonBridge = window["JsonBridge"];

            if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
                val jsPromise = jsonBridge.call<val>("clearHistory");
                val result = jsPromise.await();

                if (!result.isUndefined() && !result.isNull()) {
                    std::string resultStr = result.as<std::string>();
                    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resultStr).toUtf8());
                    QJsonObject obj = doc.object();
                    success = obj["success"].toBool();
                }
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to clear history:" << e.what();
        } catch (...) {
            qWarning() << "Failed to clear history";
        }
#else
        // Desktop native implementation
        success = saveHistoryToFile(QJsonArray());
#endif

        // Emit signal on main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            emit historyCleared(success);
        }, Qt::QueuedConnection);

        promise.addResult(QVariant::fromValue(success));
        promise.finish();
        return future;
    });
}

bool JsonBridge::isHistoryAvailable()
{
#ifdef __EMSCRIPTEN__
    try {
        val window = val::global("window");
        val jsonBridge = window["JsonBridge"];

        if (!jsonBridge.isUndefined() && !jsonBridge.isNull()) {
            return jsonBridge.call<bool>("isHistoryAvailable");
        }
    } catch (...) {
        qWarning() << "Failed to check history availability";
    }
    return false;
#else
    // Desktop native implementation - history is always available
    return true;
#endif
}
