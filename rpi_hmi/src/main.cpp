#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "mqttclient.h"

int main(int argc, char *argv[])
{
    // Application metadata
    QGuiApplication::setApplicationName("Mosquitto ESP32 HMI");
    QGuiApplication::setOrganizationName("IoTProjects");
    QGuiApplication::setApplicationVersion("2.0.0");

    // Default to Basic style for embedded Linux if not specified
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle("Basic");
    }

    QGuiApplication app(argc, argv);

    // Instantiate mqttClient BEFORE QQmlApplicationEngine so engine cleans up first
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
