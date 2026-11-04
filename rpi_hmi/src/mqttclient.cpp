#include "mqttclient.h"
#include <QDebug>

MqttClient::MqttClient(QObject *parent)
    : QObject(parent)
    , m_host("mqtt3.thingspeak.com")
    , m_port(1883)
    , m_clientId("")
    , m_username("")
    , m_password("")
    , m_channelId("")
    , m_writeApiKey("")
    , m_isConnected(false)
    , m_userInitiatedDisconnect(false)
    , m_ledState(-1)
    , m_cooldownSeconds(0)
    , m_statusMessage("Ready to connect")
    , m_subscribeFeedback(false)
    , m_autoReconnect(true)
    , m_nextPacketId(1)
{
    connect(&m_socket, &QSslSocket::connected, this, &MqttClient::onSocketConnected);
    connect(&m_socket, &QSslSocket::disconnected, this, &MqttClient::onSocketDisconnected);
    connect(&m_socket, &QSslSocket::readyRead, this, &MqttClient::onSocketReadyRead);
    connect(&m_socket, &QSslSocket::errorOccurred, this, &MqttClient::onSocketError);

    connect(&m_pingTimer, &QTimer::timeout, this, &MqttClient::sendPing);

    connect(&m_cooldownTimer, &QTimer::timeout, this, &MqttClient::onCooldownTick);
    m_cooldownTimer.setInterval(1000);

    connect(&m_reconnectTimer, &QTimer::timeout, this, &MqttClient::onReconnectTimeout);
    m_reconnectTimer.setSingleShot(true);

    loadSettings();
}

MqttClient::~MqttClient()
{
    m_reconnectTimer.stop();
    m_pingTimer.stop();
    m_cooldownTimer.stop();

    if (m_isConnected) {
        sendDisconnectPacket();
        m_socket.flush();
        m_socket.disconnectFromHost();
    }
}

void MqttClient::setHost(const QString &host)
{
    if (m_host != host) {
        m_host = host;
        emit hostChanged();
    }
}

void MqttClient::setPort(int port)
{
    if (m_port != port) {
        m_port = port;
        emit portChanged();
    }
}

void MqttClient::setClientId(const QString &clientId)
{
    if (m_clientId != clientId) {
        m_clientId = clientId;
        emit clientIdChanged();
    }
}

void MqttClient::setUsername(const QString &username)
{
    if (m_username != username) {
        m_username = username;
        emit usernameChanged();
    }
}

void MqttClient::setPassword(const QString &password)
{
    if (m_password != password) {
        m_password = password;
        emit passwordChanged();
    }
}

void MqttClient::setChannelId(const QString &channelId)
{
    if (m_channelId != channelId) {
        m_channelId = channelId;
        emit channelIdChanged();
    }
}

void MqttClient::setWriteApiKey(const QString &key)
{
    if (m_writeApiKey != key) {
        m_writeApiKey = key;
        emit writeApiKeyChanged();
    }
}

void MqttClient::setSubscribeFeedback(bool subscribe)
{
    if (m_subscribeFeedback != subscribe) {
        m_subscribeFeedback = subscribe;
        emit subscribeFeedbackChanged();
    }
}

void MqttClient::setAutoReconnect(bool autoRec)
{
    if (m_autoReconnect != autoRec) {
        m_autoReconnect = autoRec;
        emit autoReconnectChanged();
    }
}

QString MqttClient::buildPublishTopic() const
{
    QString base = QString("channels/%1/publish").arg(m_channelId.trimmed());
    if (!m_writeApiKey.trimmed().isEmpty()) {
        return QString("%1/%2").arg(base, m_writeApiKey.trimmed());
    }
    return base;
}

void MqttClient::connectToBroker()
{
    m_userInitiatedDisconnect = false;
    m_reconnectTimer.stop();

    if (m_isConnected) {
        setStatus("Already connected.");
        return;
    }

    if (m_channelId.trimmed().isEmpty()) {
        setStatus("Error: Channel ID cannot be empty.");
        return;
    }

    if (m_clientId.trimmed().isEmpty()) {
        setStatus("Error: MQTT Device Client ID required.");
        return;
    }

    m_rxBuffer.clear();
    setStatus(QString("Connecting to %1:%2...").arg(m_host).arg(m_port));

    if (m_port == 8883) {
        m_socket.connectToHostEncrypted(m_host, static_cast<quint16>(m_port));
    } else {
        m_socket.connectToHost(m_host, static_cast<quint16>(m_port));
    }
}

void MqttClient::disconnectFromBroker()
{
    m_userInitiatedDisconnect = true;
    m_reconnectTimer.stop();
    m_pingTimer.stop();

    if (!m_isConnected && m_socket.state() == QAbstractSocket::UnconnectedState) {
        setStatus("Disconnected.");
        return;
    }

    if (m_isConnected) {
        sendDisconnectPacket();
        m_socket.flush();
    }
    m_socket.disconnectFromHost();
    m_isConnected = false;
    emit isConnectedChanged();
    setStatus("Disconnected by user.");
}

void MqttClient::turnOnLed()
{
    if (!m_isConnected) {
        setStatus("Cannot send command: Broker not connected! Reconnecting...");
        connectToBroker();
        return;
    }

    if (m_cooldownSeconds > 0) {
        setStatus(QString("ThingSpeak Rate Limit: Please wait %1s before next update.").arg(m_cooldownSeconds));
        return;
    }

    QString topic = buildPublishTopic();
    QString payload = "field1=1&status=MQTTPUBLISH";

    sendPublishPacket(topic, payload);
    m_ledState = 1;
    emit ledStateChanged();
    startCooldown(15);
    setStatus(QString("LED ON command published to ThingSpeak (%1)").arg(topic));
}

void MqttClient::turnOffLed()
{
    if (!m_isConnected) {
        setStatus("Cannot send command: Broker not connected! Reconnecting...");
        connectToBroker();
        return;
    }

    if (m_cooldownSeconds > 0) {
        setStatus(QString("ThingSpeak Rate Limit: Please wait %1s before next update.").arg(m_cooldownSeconds));
        return;
    }

    QString topic = buildPublishTopic();
    QString payload = "field1=0&status=MQTTPUBLISH";

    sendPublishPacket(topic, payload);
    m_ledState = 0;
    emit ledStateChanged();
    startCooldown(15);
    setStatus(QString("LED OFF command published to ThingSpeak (%1)").arg(topic));
}

void MqttClient::publishCustom(const QString &field, const QString &value)
{
    if (!m_isConnected) {
        setStatus("Cannot publish: Broker not connected!");
        return;
    }

    QString topic = buildPublishTopic();
    QString payload = QString("%1=%2&status=MQTTPUBLISH").arg(field, value);

    sendPublishPacket(topic, payload);
    startCooldown(15);
    setStatus(QString("Published '%1' to %2").arg(payload, topic));
}

void MqttClient::onSocketConnected()
{
    setStatus("TCP connection established. Sending MQTT CONNECT...");
    sendConnectPacket();
}

void MqttClient::onSocketDisconnected()
{
    m_pingTimer.stop();
    bool wasConnected = m_isConnected;
    m_isConnected = false;
    if (wasConnected) {
        emit isConnectedChanged();
    }

    if (!m_userInitiatedDisconnect && m_autoReconnect) {
        setStatus("Connection lost. Auto-reconnecting in 5s...");
        m_reconnectTimer.start(5000);
    } else {
        setStatus("Connection closed.");
    }
}

void MqttClient::onSocketError(QAbstractSocket::SocketError error)
{
    m_pingTimer.stop();
    m_isConnected = false;
    emit isConnectedChanged();

    if (error == QAbstractSocket::RemoteHostClosedError) {
        setStatus("Remote host closed connection. (Note: Ensure ESP32 and HMI use different Client IDs in ThingSpeak!)");
    } else {
        setStatus(QString("Network Error: %1").arg(m_socket.errorString()));
    }

    if (!m_userInitiatedDisconnect && m_autoReconnect) {
        m_reconnectTimer.start(5000);
    }
}

void MqttClient::onReconnectTimeout()
{
    if (!m_isConnected && !m_userInitiatedDisconnect) {
        qDebug() << "[ThingSpeak HMI] Attempting automatic reconnection...";
        connectToBroker();
    }
}

void MqttClient::sendPing()
{
    if (m_isConnected && m_socket.state() == QAbstractSocket::ConnectedState) {
        const char pingReq[] = { static_cast<char>(0xC0), 0x00 };
        m_socket.write(pingReq, 2);
        m_socket.flush();
    }
}

void MqttClient::onCooldownTick()
{
    if (m_cooldownSeconds > 0) {
        m_cooldownSeconds--;
        emit cooldownSecondsChanged();
        if (m_cooldownSeconds == 0) {
            m_cooldownTimer.stop();
        }
    }
}

void MqttClient::startCooldown(int seconds)
{
    m_cooldownSeconds = seconds;
    emit cooldownSecondsChanged();
    m_cooldownTimer.start();
}

void MqttClient::setStatus(const QString &msg)
{
    m_statusMessage = msg;
    emit statusMessageChanged();
    qDebug() << "[ThingSpeak HMI]" << msg;
}

QByteArray MqttClient::encodeRemainingLength(int length)
{
    QByteArray bytes;
    do {
        quint8 encodedByte = length % 128;
        length /= 128;
        if (length > 0) {
            encodedByte |= 128;
        }
        bytes.append(static_cast<char>(encodedByte));
    } while (length > 0);
    return bytes;
}

QByteArray MqttClient::encodeString(const QString &str)
{
    QByteArray utf8 = str.toUtf8();
    quint16 len = static_cast<quint16>(utf8.length());
    QByteArray res;
    res.append(static_cast<char>((len >> 8) & 0xFF));
    res.append(static_cast<char>(len & 0xFF));
    res.append(utf8);
    return res;
}

void MqttClient::sendConnectPacket()
{
    QByteArray varHeader;
    varHeader.append(encodeString("MQTT"));
    varHeader.append(static_cast<char>(0x04)); // Protocol Level 4 (MQTT 3.1.1)

    quint8 flags = 0x02; // Clean Session
    bool hasUser = !m_username.isEmpty();
    bool hasPass = !m_password.isEmpty();

    if (hasUser) flags |= 0x80;
    if (hasPass) flags |= 0x40;
    varHeader.append(static_cast<char>(flags));

    // Keep Alive (60 seconds)
    varHeader.append(static_cast<char>(0x00));
    varHeader.append(static_cast<char>(0x3C));

    // Payload
    QByteArray payload;
    payload.append(encodeString(m_clientId.trimmed()));
    if (hasUser) {
        payload.append(encodeString(m_username.trimmed()));
    }
    if (hasPass) {
        payload.append(encodeString(m_password.trimmed()));
    }

    QByteArray packet;
    packet.append(static_cast<char>(0x10)); // CONNECT
    packet.append(encodeRemainingLength(varHeader.size() + payload.size()));
    packet.append(varHeader);
    packet.append(payload);

    m_socket.write(packet);
    m_socket.flush();
}

void MqttClient::sendPublishPacket(const QString &topic, const QString &payload)
{
    QByteArray varHeader = encodeString(topic);
    QByteArray payloadBytes = payload.toUtf8();

    QByteArray packet;
    packet.append(static_cast<char>(0x30)); // PUBLISH QoS 0
    packet.append(encodeRemainingLength(varHeader.size() + payloadBytes.size()));
    packet.append(varHeader);
    packet.append(payloadBytes);

    m_socket.write(packet);
    m_socket.flush();
}

void MqttClient::sendSubscribePacket(const QString &topic)
{
    quint16 packetId = m_nextPacketId++;
    if (m_nextPacketId == 0) m_nextPacketId = 1;

    QByteArray varHeader;
    varHeader.append(static_cast<char>((packetId >> 8) & 0xFF));
    varHeader.append(static_cast<char>(packetId & 0xFF));

    QByteArray payload;
    payload.append(encodeString(topic));
    payload.append(static_cast<char>(0x00)); // QoS 0

    QByteArray packet;
    packet.append(static_cast<char>(0x82)); // SUBSCRIBE
    packet.append(encodeRemainingLength(varHeader.size() + payload.size()));
    packet.append(varHeader);
    packet.append(payload);

    m_socket.write(packet);
    m_socket.flush();
    qDebug() << "Sent SUBSCRIBE to" << topic;
}

void MqttClient::sendDisconnectPacket()
{
    const char disconnectPacket[] = { static_cast<char>(0xE0), 0x00 };
    m_socket.write(disconnectPacket, 2);
    m_socket.flush();
}

void MqttClient::onSocketReadyRead()
{
    m_rxBuffer.append(m_socket.readAll());

    while (!m_rxBuffer.isEmpty()) {
        if (m_rxBuffer.size() < 2) return;

        quint8 header = static_cast<quint8>(m_rxBuffer[0]);
        quint8 type = header >> 4;
        quint8 flags = header & 0x0F;

        int multiplier = 1;
        int remLen = 0;
        int offset = 1;
        quint8 digit;
        do {
            if (offset >= m_rxBuffer.size()) return; // Wait for more data
            digit = static_cast<quint8>(m_rxBuffer[offset++]);
            remLen += (digit & 127) * multiplier;
            multiplier *= 128;
            if (multiplier > 128 * 128 * 128) {
                // Malformed remaining length
                m_rxBuffer.clear();
                return;
            }
        } while ((digit & 128) != 0);

        int totalPacketSize = offset + remLen;
        if (m_rxBuffer.size() < totalPacketSize) {
            return; // Wait for full packet
        }

        QByteArray packetData = m_rxBuffer.mid(offset, remLen);
        m_rxBuffer.remove(0, totalPacketSize);

        parseIncomingPacket(type, flags, packetData);
    }
}

void MqttClient::parseIncomingPacket(quint8 type, quint8 flags, const QByteArray &data)
{
    Q_UNUSED(flags)
    if (type == 2) { // CONNACK
        if (data.size() >= 2) {
            quint8 returnCode = static_cast<quint8>(data[1]);
            if (returnCode == 0) {
                m_isConnected = true;
                emit isConnectedChanged();
                m_pingTimer.start(25000);
                setStatus("Connected to ThingSpeak MQTT Broker!");

                if (m_subscribeFeedback && !m_channelId.trimmed().isEmpty()) {
                    QString subTopic = QString("channels/%1/subscribe/fields/field1").arg(m_channelId.trimmed());
                    sendSubscribePacket(subTopic);
                }
            } else {
                QString err = "Connection rejected: ";
                switch (returnCode) {
                    case 1: err += "Unacceptable protocol version"; break;
                    case 2: err += "Identifier rejected"; break;
                    case 3: err += "Server unavailable"; break;
                    case 4: err += "Bad username or password"; break;
                    case 5: err += "Not authorized"; break;
                    default: err += QString("Code %1").arg(returnCode); break;
                }
                setStatus(err);
                m_socket.disconnectFromHost();
            }
        }
    } else if (type == 9) { // SUBACK
        if (data.size() >= 3) {
            quint8 returnCode = static_cast<quint8>(data[2]);
            if (returnCode == 0x80) {
                setStatus("Subscription failed (Code 0x80 - Not Authorized to read channel).");
            } else {
                setStatus("Subscribed to channel updates.");
            }
        } else {
            setStatus("Subscribed to channel updates.");
        }
    } else if (type == 3) { // PUBLISH
        if (data.size() >= 2) {
            quint16 topicLen = (static_cast<quint8>(data[0]) << 8) | static_cast<quint8>(data[1]);
            if (data.size() >= 2 + topicLen) {
                QString topic = QString::fromUtf8(data.mid(2, topicLen));
                int payloadOffset = 2 + topicLen;
                quint8 qos = (flags >> 1) & 0x03;
                if (qos > 0 && data.size() >= payloadOffset + 2) {
                    quint8 pIdMsb = static_cast<quint8>(data[payloadOffset]);
                    quint8 pIdLsb = static_cast<quint8>(data[payloadOffset + 1]);
                    payloadOffset += 2;

                    // Send PUBACK
                    char puback[4] = { 0x40, 0x02, static_cast<char>(pIdMsb), static_cast<char>(pIdLsb) };
                    m_socket.write(puback, 4);
                    m_socket.flush();
                }

                QString payload = QString::fromUtf8(data.mid(payloadOffset)).trimmed();
                emit messageReceived(topic, payload);

                // Update LED state if message is from our subscribed field
                if (payload == "1" || payload.toUpper() == "ON" || payload.contains("field1=1")) {
                    m_ledState = 1;
                    emit ledStateChanged();
                    setStatus("Cloud sync: LED is ON (field1=1)");
                } else if (payload == "0" || payload.toUpper() == "OFF" || payload.contains("field1=0")) {
                    m_ledState = 0;
                    emit ledStateChanged();
                    setStatus("Cloud sync: LED is OFF (field1=0)");
                }
            }
        }
    } else if (type == 13) { // PINGRESP
        // Ping response received successfully
    }
}

void MqttClient::loadSettings()
{
    QSettings settings("IoTProjects", "ThingSpeakHMI");
    m_host = settings.value("host", "mqtt3.thingspeak.com").toString();
    m_port = settings.value("port", 1883).toInt();
    m_clientId = settings.value("clientId", "").toString();
    m_username = settings.value("username", "").toString();
    m_password = settings.value("password", "").toString();
    m_channelId = settings.value("channelId", "").toString();
    m_writeApiKey = settings.value("writeApiKey", "").toString();
    m_subscribeFeedback = settings.value("subscribeFeedback", false).toBool();
    m_autoReconnect = settings.value("autoReconnect", true).toBool();

    emit hostChanged();
    emit portChanged();
    emit clientIdChanged();
    emit usernameChanged();
    emit passwordChanged();
    emit channelIdChanged();
    emit writeApiKeyChanged();
    emit subscribeFeedbackChanged();
    emit autoReconnectChanged();
}

void MqttClient::saveSettings()
{
    QSettings settings("IoTProjects", "ThingSpeakHMI");
    settings.setValue("host", m_host);
    settings.setValue("port", m_port);
    settings.setValue("clientId", m_clientId);
    settings.setValue("username", m_username);
    settings.setValue("password", m_password);
    settings.setValue("channelId", m_channelId);
    settings.setValue("writeApiKey", m_writeApiKey);
    settings.setValue("subscribeFeedback", m_subscribeFeedback);
    settings.setValue("autoReconnect", m_autoReconnect);
    setStatus("Settings saved successfully.");
}
