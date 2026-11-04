import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 520
    title: "ESP32 LED Controller - Raspberry Pi CM5 Lite HMI"
    color: "#121820"

    // High contrast modern dark palette
    readonly property color bgDark: "#121820"
    readonly property color cardBg: "#1E2633"
    readonly property color cardBorder: "#2D3A4F"
    readonly property color accentGreen: "#00E676"
    readonly property color accentRed: "#FF3D71"
    readonly property color accentBlue: "#00B0FF"
    readonly property color textPrimary: "#FFFFFF"
    readonly property color textSecondary: "#9AA5B8"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ====================================================================
        // Top Header
        // ====================================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: root.cardBg
            radius: 12
            border.color: root.cardBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 16

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "ESP32 LED CONTROLLER"
                        font.pixelSize: 18
                        font.bold: true
                        color: root.textPrimary
                        font.family: "Monospace, sans-serif"
                    }
                    Text {
                        text: "Raspberry Pi CM5 Lite  •  Mosquitto MQTT (" + (mqttClient ? mqttClient.host : "...") + ":" + (mqttClient ? mqttClient.port : 1883) + ")"
                        font.pixelSize: 12
                        color: root.textSecondary
                    }
                }

                Item { Layout.fillWidth: true }

                // ESP32 Hardware Status Badge
                Rectangle {
                    Layout.preferredHeight: 34
                    Layout.preferredWidth: espStatusRow.width + 20
                    radius: 17
                    color: mqttClient && mqttClient.espOnline ? "#0E3320" : "#2A2A2E"
                    border.color: mqttClient && mqttClient.espOnline ? root.accentGreen : "#555A64"
                    border.width: 1

                    Row {
                        id: espStatusRow
                        anchors.centerIn: parent
                        spacing: 6

                        Text {
                            text: "ESP32:"
                            font.pixelSize: 11
                            color: root.textSecondary
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: mqttClient && mqttClient.espOnline ? "ONLINE" : "OFFLINE / UNKNOWN"
                            font.pixelSize: 11
                            font.bold: true
                            color: mqttClient && mqttClient.espOnline ? root.accentGreen : "#8892A4"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                // Mosquitto Broker Connection Badge
                Rectangle {
                    Layout.preferredHeight: 34
                    Layout.preferredWidth: statusRow.width + 24
                    radius: 17
                    color: mqttClient ? (mqttClient.isConnected ? "#0B3824" : "#3B1824") : "#1E2633"
                    border.color: mqttClient ? (mqttClient.isConnected ? root.accentGreen : root.accentRed) : root.cardBorder
                    border.width: 1

                    Row {
                        id: statusRow
                        anchors.centerIn: parent
                        spacing: 8

                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: (mqttClient && mqttClient.isConnected) ? root.accentGreen : root.accentRed
                            anchors.verticalCenter: parent.verticalCenter

                            SequentialAnimation on opacity {
                                running: mqttClient && mqttClient.isConnected
                                loops: Animation.Infinite
                                NumberAnimation { from: 1.0; to: 0.3; duration: 1000 }
                                NumberAnimation { from: 0.3; to: 1.0; duration: 1000 }
                            }
                        }

                        Text {
                            text: (mqttClient && mqttClient.isConnected) ? "BROKER CONNECTED" : "BROKER DISCONNECTED"
                            font.pixelSize: 11
                            font.bold: true
                            color: (mqttClient && mqttClient.isConnected) ? root.accentGreen : root.accentRed
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                // Connect / Disconnect button
                Button {
                    id: btnConnectToggle
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: 110
                    text: (mqttClient && mqttClient.isConnected) ? "Disconnect" : "Connect"
                    onClicked: {
                        if (!mqttClient) return
                        if (mqttClient.isConnected) {
                            mqttClient.disconnectFromBroker()
                        } else {
                            if (mqttClient.host === "") {
                                settingsDialog.open()
                            } else {
                                mqttClient.connectToBroker()
                            }
                        }
                    }

                    contentItem: Text {
                        text: btnConnectToggle.text
                        color: root.textPrimary
                        font.bold: true
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 8
                        color: btnConnectToggle.down ? "#223145" : ((mqttClient && mqttClient.isConnected) ? "#422830" : "#1B4D3E")
                        border.color: (mqttClient && mqttClient.isConnected) ? root.accentRed : root.accentGreen
                        border.width: 1
                    }
                }

                // Settings button
                Button {
                    id: btnSettings
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: 90
                    text: "Settings"
                    onClicked: settingsDialog.open()

                    contentItem: Text {
                        text: "⚙ Settings"
                        color: root.textPrimary
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 8
                        color: btnSettings.down ? "#35445B" : "#2A3649"
                        border.color: root.cardBorder
                        border.width: 1
                    }
                }
            }
        }

        // ====================================================================
        // Center Area: LED Indicator & Control Buttons
        // ====================================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: root.cardBg
            radius: 12
            border.color: root.cardBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 32

                // Left Column: Physical LED State Visualizer
                Rectangle {
                    Layout.preferredWidth: 280
                    Layout.fillHeight: true
                    color: "#161D27"
                    radius: 10
                    border.color: root.cardBorder

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 16

                        Text {
                            text: "ESP32 IN-BUILT LED"
                            font.pixelSize: 13
                            font.bold: true
                            color: root.textSecondary
                            Layout.alignment: Qt.AlignHCenter
                        }

                        // Big Glowing LED Bulb
                        Rectangle {
                            id: ledBulb
                            width: 120
                            height: 120
                            radius: 60
                            Layout.alignment: Qt.AlignHCenter

                            readonly property bool isLedOn: mqttClient && mqttClient.ledState === 1
                            readonly property bool isLedOff: mqttClient && mqttClient.ledState === 0

                            color: isLedOn ? "#00E676" : (isLedOff ? "#202A38" : "#374151")
                            border.color: isLedOn ? "#69F0AE" : "#3B4758"
                            border.width: 3

                            // Outer glow halo when LED is ON
                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width + 26
                                height: parent.height + 26
                                radius: width / 2
                                color: "transparent"
                                border.color: ledBulb.isLedOn ? "#4400E676" : "transparent"
                                border.width: 10
                                visible: ledBulb.isLedOn
                            }

                            Text {
                                anchors.centerIn: parent
                                text: ledBulb.isLedOn ? "ON" : (ledBulb.isLedOff ? "OFF" : "?")
                                font.pixelSize: 28
                                font.bold: true
                                color: ledBulb.isLedOn ? "#042211" : root.textSecondary
                            }
                        }

                        Text {
                            text: (mqttClient && mqttClient.ledState === 1) ? "GPIO 2: HIGH (Illuminated)" :
                                  ((mqttClient && mqttClient.ledState === 0) ? "GPIO 2: LOW (Extinguished)" : "Waiting for state...")
                            font.pixelSize: 13
                            font.bold: true
                            color: (mqttClient && mqttClient.ledState === 1) ? root.accentGreen :
                                   ((mqttClient && mqttClient.ledState === 0) ? root.textSecondary : root.accentBlue)
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Text {
                            text: "Topic: " + (mqttClient ? mqttClient.statusTopic : "")
                            font.pixelSize: 11
                            color: "#68768A"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Right Column: Dual Buttons (ON / OFF)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 20

                    Text {
                        text: "HMI CONTROL ACTIONS"
                        font.pixelSize: 13
                        font.bold: true
                        color: root.textSecondary
                    }

                    // 2 Big Buttons Row
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 140
                        spacing: 20

                        // BUTTON 1: TURN ON LED
                        Button {
                            id: btnOn
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            enabled: mqttClient && mqttClient.isConnected

                            onClicked: {
                                if (mqttClient) mqttClient.turnOnLed()
                            }

                            contentItem: ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 8

                                Text {
                                    text: "⚡ TURN ON LED"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: btnOn.enabled ? "#FFFFFF" : "#5A6578"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Text {
                                    text: "Publish: " + (mqttClient ? mqttClient.commandTopic : "esp32/led/set") + " ➔ 'ON'"
                                    font.pixelSize: 12
                                    color: btnOn.enabled ? "#B9F6CA" : "#4A5568"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }

                            background: Rectangle {
                                radius: 12
                                color: btnOn.down ? "#00A854" : (btnOn.enabled ? "#00B359" : "#1B222E")
                                border.color: btnOn.enabled ? root.accentGreen : "#2E3846"
                                border.width: btnOn.enabled ? 2 : 1

                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                        }

                        // BUTTON 2: TURN OFF LED
                        Button {
                            id: btnOff
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            enabled: mqttClient && mqttClient.isConnected

                            onClicked: {
                                if (mqttClient) mqttClient.turnOffLed()
                            }

                            contentItem: ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 8

                                Text {
                                    text: "⏹ TURN OFF LED"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: btnOff.enabled ? "#FFFFFF" : "#5A6578"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Text {
                                    text: "Publish: " + (mqttClient ? mqttClient.commandTopic : "esp32/led/set") + " ➔ 'OFF'"
                                    font.pixelSize: 12
                                    color: btnOff.enabled ? "#FFB4C4" : "#4A5568"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }

                            background: Rectangle {
                                radius: 12
                                color: btnOff.down ? "#C41C48" : (btnOff.enabled ? "#D81B4E" : "#1B222E")
                                border.color: btnOff.enabled ? root.accentRed : "#2E3846"
                                border.width: btnOff.enabled ? 2 : 1

                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                        }
                    }

                    // Mosquitto Protocol & Latency Notice
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52
                        radius: 8
                        color: "#161D27"
                        border.color: root.cardBorder

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 12

                            Text {
                                text: "⚡"
                                font.pixelSize: 18
                            }

                            Text {
                                text: "Mosquitto MQTT Local Broker  •  Sub-millisecond latency  •  No cloud limits"
                                color: root.accentBlue
                                font.pixelSize: 12
                                Layout.fillWidth: true
                            }

                            Text {
                                text: "QoS 0 Direct"
                                color: root.textSecondary
                                font.pixelSize: 11
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // ====================================================================
        // Bottom Status Bar
        // ====================================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: root.cardBg
            radius: 8
            border.color: root.cardBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: "STATUS:"
                    font.pixelSize: 11
                    font.bold: true
                    color: root.textSecondary
                }

                Text {
                    id: statusLabel
                    text: mqttClient ? mqttClient.statusMessage : "Initializing..."
                    color: root.textPrimary
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }
    }

    // ========================================================================
    // Settings Dialog
    // ========================================================================
    Dialog {
        id: settingsDialog
        title: "Mosquitto MQTT Configuration"
        anchors.centerIn: parent
        width: 480
        modal: true
        focus: true

        background: Rectangle {
            color: "#1E2633"
            radius: 12
            border.color: root.cardBorder
            border.width: 1
        }

        header: Rectangle {
            height: 48
            color: "#161D27"
            radius: 12

            Text {
                anchors.centerIn: parent
                text: "Mosquitto MQTT Connection Settings"
                color: root.textPrimary
                font.bold: true
                font.pixelSize: 14
            }
        }

        contentItem: ColumnLayout {
            spacing: 12

            Text {
                text: "Mosquitto Broker Host / IP (e.g. Ubuntu PC IP or localhost):"
                color: root.textSecondary
                font.pixelSize: 11
            }
            TextField {
                id: txtHost
                Layout.fillWidth: true
                text: mqttClient ? mqttClient.host : "192.168.1.100"
                color: root.textPrimary
                background: Rectangle {
                    color: "#161D27"
                    radius: 6
                    border.color: root.cardBorder
                }
            }

            RowLayout {
                spacing: 12
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.fillWidth: true
                    Text { text: "Port:"; color: root.textSecondary; font.pixelSize: 11 }
                    TextField {
                        id: txtPort
                        Layout.fillWidth: true
                        text: mqttClient ? String(mqttClient.port) : "1883"
                        color: root.textPrimary
                        background: Rectangle { color: "#161D27"; radius: 6; border.color: root.cardBorder }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Text { text: "Client ID:"; color: root.textSecondary; font.pixelSize: 11 }
                    TextField {
                        id: txtClientId
                        Layout.fillWidth: true
                        text: mqttClient ? mqttClient.clientId : "rpi_cm5_hmi"
                        color: root.textPrimary
                        background: Rectangle { color: "#161D27"; radius: 6; border.color: root.cardBorder }
                    }
                }
            }

            Text {
                text: "Command Topic (Publish):"
                color: root.textSecondary
                font.pixelSize: 11
            }
            TextField {
                id: txtCommandTopic
                Layout.fillWidth: true
                text: mqttClient ? mqttClient.commandTopic : "esp32/led/set"
                color: root.textPrimary
                background: Rectangle { color: "#161D27"; radius: 6; border.color: root.cardBorder }
            }

            Text {
                text: "Status Topic (Subscribe feedback):"
                color: root.textSecondary
                font.pixelSize: 11
            }
            TextField {
                id: txtStatusTopic
                Layout.fillWidth: true
                text: mqttClient ? mqttClient.statusTopic : "esp32/led/status"
                color: root.textPrimary
                background: Rectangle { color: "#161D27"; radius: 6; border.color: root.cardBorder }
            }
        }

        footer: DialogButtonBox {
            background: Rectangle { color: "transparent" }

            Button {
                text: "Cancel"
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: settingsDialog.close()
                contentItem: Text { text: "Cancel"; color: root.textSecondary; horizontalAlignment: Text.AlignHCenter }
                background: Rectangle { color: "#2A3649"; radius: 6 }
            }

            Button {
                text: "Save & Connect"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                contentItem: Text { text: "Save & Connect"; color: "#FFFFFF"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                background: Rectangle { color: "#1B4D3E"; radius: 6; border.color: root.accentGreen }

                onClicked: {
                    if (mqttClient) {
                        mqttClient.host = txtHost.text.trim()
                        mqttClient.port = parseInt(txtPort.text.trim()) || 1883
                        mqttClient.clientId = txtClientId.text.trim()
                        mqttClient.commandTopic = txtCommandTopic.text.trim()
                        mqttClient.statusTopic = txtStatusTopic.text.trim()
                        mqttClient.saveSettings()

                        if (mqttClient.isConnected) {
                            mqttClient.disconnectFromBroker()
                        }
                        mqttClient.connectToBroker()
                    }
                    settingsDialog.close()
                }
            }
        }
    }
}
