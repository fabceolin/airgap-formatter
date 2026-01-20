#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Set application metadata
    QGuiApplication::setApplicationName("Airgap JSON Formatter");
    QGuiApplication::setOrganizationName("Airgap");
    QGuiApplication::setApplicationVersion("0.1.0");

    // Use Fusion style for consistent look across platforms
    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;

    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/AirgapFormatter/qml/Main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
