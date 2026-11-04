#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include "mqttclient.h"

int main(int argc, char *argv[])
{
    // Enable High DPI scaling
    QGuiApplication::setApplicationName("ThingSpeak ESP32 HMI");
    QGuiApplication::setOrganizationName("IoTProjects");
    QGuiApplication::setApplicationVersion("1.0.0");

    // Default to Basic / Fusion quick style for embedded systems if not set
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle("Basic");
    }

    QGuiApplication app(argc, argv);

    // Declare mqttClient BEFORE engine so engine is destroyed FIRST on exit,
    // avoiding QML null property access warnings during teardown.
    MqttClient mqttClient;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("mqttClient", &mqttClient);

    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
