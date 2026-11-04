#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <QSettings>

/**
 * @brief Standalone lightweight MQTT 3.1.1 client implementation using QTcpSocket.
 *
 * Implements core MQTT protocol frames without external library dependencies,
 * ensuring high portability on embedded Linux (e.g. Raspberry Pi OS Lite).
 */
class MqttClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString clientId READ clientId WRITE setClientId NOTIFY clientIdChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString commandTopic READ commandTopic WRITE setCommandTopic NOTIFY commandTopicChanged)
    Q_PROPERTY(QString statusTopic READ statusTopic WRITE setStatusTopic NOTIFY statusTopicChanged)
    Q_PROPERTY(QString deviceStatusTopic READ deviceStatusTopic WRITE setDeviceStatusTopic NOTIFY deviceStatusTopicChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)
    Q_PROPERTY(bool espOnline READ espOnline NOTIFY espOnlineChanged)
    Q_PROPERTY(int ledState READ ledState NOTIFY ledStateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool autoReconnect READ autoReconnect WRITE setAutoReconnect NOTIFY autoReconnectChanged)

public:
    explicit MqttClient(QObject *parent = nullptr);
    ~MqttClient() override;

    QString host() const { return m_host; }
    void setHost(const QString &host);

    int port() const { return m_port; }
    void setPort(int port);

    QString clientId() const { return m_clientId; }
    void setClientId(const QString &clientId);

    QString username() const { return m_username; }
    void setUsername(const QString &username);

    QString password() const { return m_password; }
    void setPassword(const QString &password);

    QString commandTopic() const { return m_commandTopic; }
    void setCommandTopic(const QString &topic);

    QString statusTopic() const { return m_statusTopic; }
    void setStatusTopic(const QString &topic);

    QString deviceStatusTopic() const { return m_deviceStatusTopic; }
    void setDeviceStatusTopic(const QString &topic);

    bool isConnected() const { return m_isConnected; }
    bool espOnline() const { return m_espOnline; }
    int ledState() const { return m_ledState; } // -1: unknown, 0: OFF, 1: ON
    QString statusMessage() const { return m_statusMessage; }

    bool autoReconnect() const { return m_autoReconnect; }
    void setAutoReconnect(bool autoRec);

    // QML Invocable controls
    Q_INVOKABLE void connectToBroker();
    Q_INVOKABLE void disconnectFromBroker();
    Q_INVOKABLE void turnOnLed();
    Q_INVOKABLE void turnOffLed();
    Q_INVOKABLE void toggleLed();
    Q_INVOKABLE void publishCustom(const QString &topic, const QString &payload);
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

signals:
    void hostChanged();
    void portChanged();
    void clientIdChanged();
    void usernameChanged();
    void passwordChanged();
    void commandTopicChanged();
    void statusTopicChanged();
    void deviceStatusTopicChanged();
    void isConnectedChanged();
    void espOnlineChanged();
    void ledStateChanged();
    void statusMessageChanged();
    void autoReconnectChanged();
    void messageReceived(const QString &topic, const QString &payload);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void sendPing();
    void onReconnectTimeout();

private:
    void setStatus(const QString &msg);
    void sendConnectPacket();
    void sendPublishPacket(const QString &topic, const QString &payload, bool retain = false);
    void sendSubscribePacket(const QString &topic);
    void sendDisconnectPacket();
    void parseIncomingPacket(quint8 type, quint8 flags, const QByteArray &data);

    static QByteArray encodeRemainingLength(int length);
    static QByteArray encodeString(const QString &str);

    QTcpSocket m_socket;
    QTimer m_pingTimer;
    QTimer m_reconnectTimer;
    QByteArray m_rxBuffer;

    QString m_host;
    int m_port;
    QString m_clientId;
    QString m_username;
    QString m_password;
    QString m_commandTopic;
    QString m_statusTopic;
    QString m_deviceStatusTopic;

    bool m_isConnected;
    bool m_userInitiatedDisconnect;
    bool m_espOnline;
    int m_ledState;
    QString m_statusMessage;
    bool m_autoReconnect;
    quint16 m_nextPacketId;
};

#endif // MQTTCLIENT_H
