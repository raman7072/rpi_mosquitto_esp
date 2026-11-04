#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QTimer>
#include <QByteArray>
#include <QSettings>

class MqttClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString clientId READ clientId WRITE setClientId NOTIFY clientIdChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString channelId READ channelId WRITE setChannelId NOTIFY channelIdChanged)
    Q_PROPERTY(QString writeApiKey READ writeApiKey WRITE setWriteApiKey NOTIFY writeApiKeyChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)
    Q_PROPERTY(int ledState READ ledState NOTIFY ledStateChanged)
    Q_PROPERTY(int cooldownSeconds READ cooldownSeconds NOTIFY cooldownSecondsChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool subscribeFeedback READ subscribeFeedback WRITE setSubscribeFeedback NOTIFY subscribeFeedbackChanged)
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

    QString channelId() const { return m_channelId; }
    void setChannelId(const QString &channelId);

    QString writeApiKey() const { return m_writeApiKey; }
    void setWriteApiKey(const QString &key);

    bool isConnected() const { return m_isConnected; }
    int ledState() const { return m_ledState; }
    int cooldownSeconds() const { return m_cooldownSeconds; }
    QString statusMessage() const { return m_statusMessage; }

    bool subscribeFeedback() const { return m_subscribeFeedback; }
    void setSubscribeFeedback(bool subscribe);

    bool autoReconnect() const { return m_autoReconnect; }
    void setAutoReconnect(bool autoRec);

    Q_INVOKABLE void connectToBroker();
    Q_INVOKABLE void disconnectFromBroker();
    Q_INVOKABLE void turnOnLed();
    Q_INVOKABLE void turnOffLed();
    Q_INVOKABLE void publishCustom(const QString &field, const QString &value);
    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();

signals:
    void hostChanged();
    void portChanged();
    void clientIdChanged();
    void usernameChanged();
    void passwordChanged();
    void channelIdChanged();
    void writeApiKeyChanged();
    void isConnectedChanged();
    void ledStateChanged();
    void cooldownSecondsChanged();
    void statusMessageChanged();
    void subscribeFeedbackChanged();
    void autoReconnectChanged();
    void messageReceived(const QString &topic, const QString &payload);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void sendPing();
    void onCooldownTick();
    void onReconnectTimeout();

private:
    void sendConnectPacket();
    void sendPublishPacket(const QString &topic, const QString &payload);
    void sendSubscribePacket(const QString &topic);
    void sendDisconnectPacket();
    void parseIncomingPacket(quint8 type, quint8 flags, const QByteArray &data);
    void setStatus(const QString &msg);
    void startCooldown(int seconds = 15);
    QString buildPublishTopic() const;

    static QByteArray encodeRemainingLength(int length);
    static QByteArray encodeString(const QString &str);

    QSslSocket m_socket;
    QByteArray m_rxBuffer;
    QTimer m_pingTimer;
    QTimer m_cooldownTimer;
    QTimer m_reconnectTimer;

    QString m_host;
    int m_port;
    QString m_clientId;
    QString m_username;
    QString m_password;
    QString m_channelId;
    QString m_writeApiKey;

    bool m_isConnected;
    bool m_userInitiatedDisconnect;
    int m_ledState; // -1: Unknown, 0: OFF, 1: ON
    int m_cooldownSeconds;
    QString m_statusMessage;
    bool m_subscribeFeedback;
    bool m_autoReconnect;
    quint16 m_nextPacketId;
};

#endif // MQTTCLIENT_H
