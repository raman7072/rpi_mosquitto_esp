#include "mqttclient.h"
#include <QDebug>
#include <QRandomGenerator>
#include <QDateTime>

MqttClient::MqttClient(QObject *parent)
    : QObject(parent)
    , m_host("192.168.1.100")
    , m_port(1883)
    , m_clientId("")
    , m_username("")
    , m_password("")
    , m_commandTopic("esp32/led/set")
    , m_statusTopic("esp32/led/status")
    , m_deviceStatusTopic("esp32/status")
    , m_isConnected(false)
    , m_userInitiatedDisconnect(false)
    , m_espOnline(false)
    , m_ledState(-1)
    , m_statusMessage("Ready to connect")
    , m_autoReconnect(true)
    , m_nextPacketId(1)
{
    connect(&m_socket, &QTcpSocket::connected, this, &MqttClient::onSocketConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &MqttClient::onSocketDisconnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &MqttClient::onSocketReadyRead);
    connect(&m_socket, &QTcpSocket::errorOccurred, this, &MqttClient::onSocketError);

    connect(&m_pingTimer, &QTimer::timeout, this, &MqttClient::sendPing);

    connect(&m_reconnectTimer, &QTimer::timeout, this, &MqttClient::onReconnectTimeout);
    m_reconnectTimer.setSingleShot(true);

    loadSettings();

    // Ensure a unique client ID if none was loaded
    if (m_clientId.trimmed().isEmpty()) {
        m_clientId = QString("rpi_cm5_hmi_%1").arg(QRandomGenerator::global()->bounded(1000, 9999));
        emit clientIdChanged();
    }
}

MqttClient::~MqttClient()
{
    m_reconnectTimer.stop();
    m_pingTimer.stop();

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

void MqttClient::setCommandTopic(const QString &topic)
{
    if (m_commandTopic != topic) {
        m_commandTopic = topic;
        emit commandTopicChanged();
    }
}

void MqttClient::setStatusTopic(const QString &topic)
{
    if (m_statusTopic != topic) {
        m_statusTopic = topic;
        emit statusTopicChanged();
    }
}

void MqttClient::setDeviceStatusTopic(const QString &topic)
{
    if (m_deviceStatusTopic != topic) {
        m_deviceStatusTopic = topic;
        emit deviceStatusTopicChanged();
    }
}

void MqttClient::setAutoReconnect(bool autoRec)
{
    if (m_autoReconnect != autoRec) {
        m_autoReconnect = autoRec;
        emit autoReconnectChanged();
    }
}

void MqttClient::connectToBroker()
{
    m_userInitiatedDisconnect = false;
    m_reconnectTimer.stop();

    if (m_isConnected) {
        setStatus("Already connected to Mosquitto broker.");
        return;
    }

    if (m_host.trimmed().isEmpty()) {
        setStatus("Error: Broker Host / IP cannot be empty.");
        return;
    }

    m_rxBuffer.clear();
    setStatus(QString("Connecting to Mosquitto at %1:%2...").arg(m_host).arg(m_port));

    m_socket.connectToHost(m_host.trimmed(), static_cast<quint16>(m_port));
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
        setStatus("Broker not connected. Attempting reconnection...");
        connectToBroker();
        return;
    }

    sendPublishPacket(m_commandTopic.trimmed(), "ON");
    m_ledState = 1;
    emit ledStateChanged();
    setStatus(QString("[%1] Sent 'ON' to %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), m_commandTopic));
}

void MqttClient::turnOffLed()
{
    if (!m_isConnected) {
        setStatus("Broker not connected. Attempting reconnection...");
        connectToBroker();
        return;
    }

    sendPublishPacket(m_commandTopic.trimmed(), "OFF");
    m_ledState = 0;
    emit ledStateChanged();
    setStatus(QString("[%1] Sent 'OFF' to %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), m_commandTopic));
}

void MqttClient::toggleLed()
{
    if (m_ledState == 1) {
        turnOffLed();
    } else {
        turnOnLed();
    }
}

void MqttClient::publishCustom(const QString &topic, const QString &payload)
{
    if (!m_isConnected) {
        setStatus("Cannot publish: Broker not connected!");
        return;
    }

    sendPublishPacket(topic, payload);
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

    if (error == QAbstractSocket::ConnectionRefusedError) {
        setStatus("Connection refused. Is Mosquitto running and configured to listen on 0.0.0.0?");
    } else if (error == QAbstractSocket::HostNotFoundError) {
        setStatus(QString("Host '%1' not found. Check IP address.").arg(m_host));
    } else if (error == QAbstractSocket::RemoteHostClosedError) {
        setStatus("Broker closed connection. Ensure client IDs are unique!");
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
        qDebug() << "[Mosquitto HMI] Attempting automatic reconnection...";
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

void MqttClient::setStatus(const QString &msg)
{
    m_statusMessage = msg;
    emit statusMessageChanged();
    qDebug() << "[Mosquitto HMI]" << msg;
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

void MqttClient::sendPublishPacket(const QString &topic, const QString &payload, bool retain)
{
    QByteArray varHeader = encodeString(topic);
    QByteArray payloadBytes = payload.toUtf8();

    quint8 headerByte = 0x30; // PUBLISH QoS 0
    if (retain) {
        headerByte |= 0x01;
    }

    QByteArray packet;
    packet.append(static_cast<char>(headerByte));
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
    qDebug() << "[Mosquitto HMI] Subscribed to topic:" << topic;
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
                // Malformed packet
                m_rxBuffer.clear();
                return;
            }
        } while ((digit & 128) != 0);

        int totalPacketSize = offset + remLen;
        if (m_rxBuffer.size() < totalPacketSize) {
            return; // Incomplete packet, wait for more chunks
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
                setStatus("Connected to Mosquitto Broker successfully!");

                // Automatically subscribe to status topics
                if (!m_statusTopic.trimmed().isEmpty()) {
                    sendSubscribePacket(m_statusTopic.trimmed());
                }
                if (!m_deviceStatusTopic.trimmed().isEmpty()) {
                    sendSubscribePacket(m_deviceStatusTopic.trimmed());
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
        setStatus("Subscribed to MQTT topics successfully.");
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

                // LED state feedback from ESP32
                if (topic == m_statusTopic.trimmed()) {
                    if (payload.compare("ON", Qt::CaseInsensitive) == 0 || payload == "1") {
                        m_ledState = 1;
                        emit ledStateChanged();
                        setStatus(QString("[%1] ESP32 Feedback: LED is ON").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
                    } else if (payload.compare("OFF", Qt::CaseInsensitive) == 0 || payload == "0") {
                        m_ledState = 0;
                        emit ledStateChanged();
                        setStatus(QString("[%1] ESP32 Feedback: LED is OFF").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
                    }
                }

                // ESP32 device online/offline status (LWT)
                if (topic == m_deviceStatusTopic.trimmed()) {
                    if (payload.compare("online", Qt::CaseInsensitive) == 0) {
                        m_espOnline = true;
                        emit espOnlineChanged();
                        setStatus("ESP32 Device is ONLINE");
                    } else if (payload.compare("offline", Qt::CaseInsensitive) == 0) {
                        m_espOnline = false;
                        emit espOnlineChanged();
                        setStatus("ESP32 Device is OFFLINE");
                    }
                }
            }
        }
    } else if (type == 13) { // PINGRESP
        // Keep-alive acknowledged
    }
}

void MqttClient::loadSettings()
{
    QSettings settings("IoTProjects", "MosquittoEspHmi");
    m_host = settings.value("host", "192.168.1.100").toString();
    m_port = settings.value("port", 1883).toInt();
    m_clientId = settings.value("clientId", "").toString();
    m_username = settings.value("username", "").toString();
    m_password = settings.value("password", "").toString();
    m_commandTopic = settings.value("commandTopic", "esp32/led/set").toString();
    m_statusTopic = settings.value("statusTopic", "esp32/led/status").toString();
    m_deviceStatusTopic = settings.value("deviceStatusTopic", "esp32/status").toString();
    m_autoReconnect = settings.value("autoReconnect", true).toBool();

    emit hostChanged();
    emit portChanged();
    emit clientIdChanged();
    emit usernameChanged();
    emit passwordChanged();
    emit commandTopicChanged();
    emit statusTopicChanged();
    emit deviceStatusTopicChanged();
    emit autoReconnectChanged();
}

void MqttClient::saveSettings()
{
    QSettings settings("IoTProjects", "MosquittoEspHmi");
    settings.setValue("host", m_host);
    settings.setValue("port", m_port);
    settings.setValue("clientId", m_clientId);
    settings.setValue("username", m_username);
    settings.setValue("password", m_password);
    settings.setValue("commandTopic", m_commandTopic);
    settings.setValue("statusTopic", m_statusTopic);
    settings.setValue("deviceStatusTopic", m_deviceStatusTopic);
    settings.setValue("autoReconnect", m_autoReconnect);
    setStatus("Settings saved successfully.");
}
