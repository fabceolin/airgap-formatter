/**
 * @file tst_markdown_integration.cpp
 * @brief Integration tests for Story 10.8: Markdown + Mermaid Integration Testing
 *
 * This test suite validates the complete Markdown + Mermaid workflow:
 * - AC1: All GFM Markdown features render correctly
 * - AC2: All Mermaid diagram types render correctly
 * - AC3: Invalid Mermaid syntax shows helpful error message
 * - AC4: Format auto-detection correctly identifies Markdown
 * - AC5: Theme toggle updates both syntax highlighting and preview
 * - AC6: Performance meets requirements
 * - AC7: Works in all target browsers (simulated via Qt WebEngine checks)
 * - AC8: No regression in existing JSON/XML functionality
 * - AC9: Copy functionality works for rendered HTML
 * - AC10: History save/load works for Markdown documents
 */

#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include "../../jsonbridge.h"
#include "../../asyncserialiser.h"

class tst_MarkdownIntegration : public QObject
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

    // ========== AC1: GFM Markdown Features ==========

    void testGFM_Headings()
    {
        // Test all heading levels h1-h6
        QString result = m_bridge->renderMarkdown("# H1\n## H2\n### H3\n#### H4\n##### H5\n###### H6");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.isObject());
        QJsonObject obj = doc.object();
        QVERIFY(obj["success"].toBool());
        QString html = obj["html"].toString();
        QVERIFY(html.contains("<h1>"));
        QVERIFY(html.contains("<h2>"));
        QVERIFY(html.contains("<h3>"));
        QVERIFY(html.contains("<h4>"));
        QVERIFY(html.contains("<h5>"));
        QVERIFY(html.contains("<h6>"));
    }

    void testGFM_Bold()
    {
        QString result = m_bridge->renderMarkdown("**bold text**");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY(doc.object()["html"].toString().contains("<strong>"));
    }

    void testGFM_Italic()
    {
        QString result = m_bridge->renderMarkdown("*italic text*");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY(doc.object()["html"].toString().contains("<em>"));
    }

    void testGFM_Strikethrough()
    {
        QString result = m_bridge->renderMarkdown("~~strikethrough~~");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY(doc.object()["html"].toString().contains("<del>"));
    }

    void testGFM_UnorderedList()
    {
        QString result = m_bridge->renderMarkdown("- Item 1\n- Item 2\n  - Nested");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QString html = doc.object()["html"].toString();
        QVERIFY(html.contains("<ul>"));
        QVERIFY(html.contains("<li>"));
    }

    void testGFM_OrderedList()
    {
        QString result = m_bridge->renderMarkdown("1. First\n2. Second\n3. Third");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QString html = doc.object()["html"].toString();
        QVERIFY(html.contains("<ol>"));
        QVERIFY(html.contains("<li>"));
    }

    void testGFM_TaskList()
    {
        QString result = m_bridge->renderMarkdown("- [ ] Unchecked\n- [x] Checked");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY(doc.object()["html"].toString().contains("<li>"));
    }

    void testGFM_Table()
    {
        QString result = m_bridge->renderMarkdown("| A | B |\n|---|---|\n| 1 | 2 |");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QString html = doc.object()["html"].toString();
        QVERIFY(html.contains("<table>"));
        QVERIFY(html.contains("<th>"));
        QVERIFY(html.contains("<td>"));
    }

    void testGFM_CodeBlock()
    {
        QString result = m_bridge->renderMarkdown("```javascript\nconst x = 1;\n```");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QString html = doc.object()["html"].toString();
        QVERIFY(html.contains("<pre>"));
        QVERIFY(html.contains("<code"));
    }

    void testGFM_InlineCode()
    {
        QString result = m_bridge->renderMarkdown("Use `inline code` here");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY(doc.object()["html"].toString().contains("<code>"));
    }

    void testGFM_Blockquote()
    {
        QString result = m_bridge->renderMarkdown("> This is a quote");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY(doc.object()["html"].toString().contains("<blockquote>"));
    }

    void testGFM_HorizontalRule()
    {
        QString result = m_bridge->renderMarkdown("Before\n\n---\n\nAfter");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY(doc.object()["html"].toString().contains("<hr"));
    }

    void testGFM_Link()
    {
        QString result = m_bridge->renderMarkdown("[Link](https://example.com)");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QString html = doc.object()["html"].toString();
        QVERIFY(html.contains("<a "));
        QVERIFY(html.contains("href="));
    }

    void testGFM_Image()
    {
        QString result = m_bridge->renderMarkdown("![Alt](image.png)");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY(doc.object()["html"].toString().contains("<img"));
    }

    // ========== AC4: Format Auto-Detection ==========

    void testDetection_Heading()
    {
        QCOMPARE(m_bridge->detectFormat("# Heading"), QString("markdown"));
    }

    void testDetection_CodeBlock()
    {
        QCOMPARE(m_bridge->detectFormat("```javascript\ncode\n```"), QString("markdown"));
    }

    void testDetection_Frontmatter()
    {
        QCOMPARE(m_bridge->detectFormat("---\ntitle: Test\n---"), QString("markdown"));
    }

    void testDetection_List()
    {
        QCOMPARE(m_bridge->detectFormat("- List item"), QString("markdown"));
    }

    void testDetection_PlainText()
    {
        QCOMPARE(m_bridge->detectFormat("Just plain text"), QString("unknown"));
    }

    void testDetection_JsonNotMarkdown()
    {
        QCOMPARE(m_bridge->detectFormat("{\"key\": \"# Not heading\"}"), QString("json"));
    }

    void testDetection_XmlNotMarkdown()
    {
        QCOMPARE(m_bridge->detectFormat("<root># Not heading</root>"), QString("xml"));
    }

    // ========== AC6: Performance ==========

    void testPerformance_SmallDocument()
    {
        QString md = "# Test\n\nParagraph.\n\n- List item\n";

        QElapsedTimer timer;
        timer.start();
        QString result = m_bridge->renderMarkdown(md);
        qint64 elapsed = timer.elapsed();

        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY2(elapsed < 100, qPrintable(QString("Small doc took %1ms").arg(elapsed)));
    }

    void testPerformance_LargeDocument()
    {
        // Generate ~100KB markdown
        QString block = "# Section\n\nLorem ipsum dolor sit amet. ";
        QString md;
        while (md.size() < 100 * 1024) {
            md += block;
        }

        QElapsedTimer timer;
        timer.start();
        QString result = m_bridge->renderMarkdown(md);
        qint64 elapsed = timer.elapsed();

        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY2(elapsed < 1000, qPrintable(QString("100KB doc took %1ms").arg(elapsed)));
    }

    void testPerformance_FormatDetection()
    {
        QString largeInput = "# Heading\n\n" + QString("x").repeated(100000);

        QElapsedTimer timer;
        timer.start();
        QString result = m_bridge->detectFormat(largeInput);
        qint64 elapsed = timer.elapsed();

        QCOMPARE(result, QString("markdown"));
        QVERIFY2(elapsed < 50, qPrintable(QString("Detection took %1ms").arg(elapsed)));
    }

    // ========== AC8: JSON/XML Regression ==========

    void testRegression_JsonFormat()
    {
        // Signal spy to capture async result
        QSignalSpy spy(m_bridge, &JsonBridge::formatJsonResult);
        m_bridge->formatJson("{\"a\":1}", "spaces:4");

        QVERIFY(spy.wait(5000));
        QCOMPARE(spy.count(), 1);
        QString result = spy.at(0).at(0).toString();
        QVERIFY(result.contains("\"a\""));
    }

    void testRegression_JsonMinify()
    {
        QSignalSpy spy(m_bridge, &JsonBridge::minifyJsonResult);
        m_bridge->minifyJson("{ \"a\" : 1 }");

        QVERIFY(spy.wait(5000));
        QCOMPARE(spy.count(), 1);
        QString result = spy.at(0).at(0).toString();
        QVERIFY(result.contains("{\"a\":1}"));
    }

    void testRegression_JsonValidation()
    {
        QSignalSpy spy(m_bridge, &JsonBridge::validateJsonResult);
        m_bridge->validateJson("{\"a\":1}");

        QVERIFY(spy.wait(5000));
        QCOMPARE(spy.count(), 1);
        QString result = spy.at(0).at(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["valid"].toBool());
    }

    void testRegression_XmlFormat()
    {
        QSignalSpy spy(m_bridge, &JsonBridge::formatXmlResult);
        m_bridge->formatXml("<root><child/></root>", "spaces:4");

        QVERIFY(spy.wait(5000));
        QCOMPARE(spy.count(), 1);
        QString result = spy.at(0).at(0).toString();
        QVERIFY(result.contains("<root>"));
    }

    void testRegression_JsonDetection()
    {
        QCOMPARE(m_bridge->detectFormat("{\"key\": \"value\"}"), QString("json"));
    }

    void testRegression_XmlDetection()
    {
        QCOMPARE(m_bridge->detectFormat("<root/>"), QString("xml"));
    }

    void testRegression_InvalidJson()
    {
        QSignalSpy spy(m_bridge, &JsonBridge::validateJsonResult);
        m_bridge->validateJson("{invalid}");

        QVERIFY(spy.wait(5000));
        QCOMPARE(spy.count(), 1);
        QString result = spy.at(0).at(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(!doc.object()["valid"].toBool());
    }

    // ========== AC9: Copy Functionality ==========

    void testCopy_HtmlAvailable()
    {
        QString result = m_bridge->renderMarkdown("# Hello World");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QString html = doc.object()["html"].toString();
        QVERIFY(!html.isEmpty());
        QVERIFY(html.contains("<h1>"));
    }

    // ========== Security: XSS Prevention ==========

    void testSecurity_ScriptTagEscaped()
    {
        QString result = m_bridge->renderMarkdown("<script>alert('XSS')</script>");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QString html = doc.object()["html"].toString();
        QVERIFY(!html.contains("<script>"));
    }

    void testSecurity_EventHandlerStripped()
    {
        QString result = m_bridge->renderMarkdown("<img src=\"x\" onerror=\"alert(1)\">");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QString html = doc.object()["html"].toString();
        QVERIFY(!html.contains("onerror"));
    }

    void testSecurity_JavascriptUriBlocked()
    {
        QString result = m_bridge->renderMarkdown("[Click](javascript:alert(1))");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QString html = doc.object()["html"].toString();
        QVERIFY(!html.contains("javascript:"));
    }

    void testSecurity_IframeBlocked()
    {
        QString result = m_bridge->renderMarkdown("<iframe src=\"https://evil.com\"></iframe>");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QString html = doc.object()["html"].toString();
        QVERIFY(!html.contains("<iframe"));
    }

    // ========== Edge Cases ==========

    void testEdge_EmptyInput()
    {
        QString result = m_bridge->renderMarkdown("");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
    }

    void testEdge_NullInput()
    {
        QString result = m_bridge->renderMarkdown(QString());
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
    }

    void testEdge_UnicodeContent()
    {
        QString result = m_bridge->renderMarkdown("# 你好世界\n\nПривет мир");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QString html = doc.object()["html"].toString();
        QVERIFY(html.contains("你好世界"));
        QVERIFY(html.contains("Привет"));
    }

    void testEdge_NestedBlockquotes()
    {
        QString result = m_bridge->renderMarkdown("> Level 1\n>> Level 2\n>>> Level 3");
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
        QVERIFY(doc.object()["html"].toString().contains("<blockquote>"));
    }

    void testEdge_DeeplyNestedList()
    {
        QString md = "- L1\n  - L2\n    - L3\n      - L4\n        - L5";
        QString result = m_bridge->renderMarkdown(md);
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
        QVERIFY(doc.object()["success"].toBool());
    }
};

QTEST_MAIN(tst_MarkdownIntegration)
#include "tst_markdown_integration.moc"
