import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 520
    title: "ESP32 Controller - Raspberry Pi CM5 Lite HMI"
    color: "#121820"

    // High contrast modern dark palette
    readonly property color bgDark: "#121820"
    readonly property color cardBg: "#1E2633"
    readonly property color cardBorder: "#2D3A4F"
    readonly property color accentGreen: "#00E676"
    readonly property color accentRed: "#FF3D71"
    readonly property color accentBlue: "#00B0FF"
    readonly property color accentOrange: "#FF9800"
    readonly property color textPrimary: "#FFFFFF"
    readonly property color textSecondary: "#9AA5B8"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Top Header
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
                        text: "Raspberry Pi CM5 Lite  •  ThingSpeak MQTT (mqtt3.thingspeak.com)"
                        font.pixelSize: 12
                        color: root.textSecondary
                    }
                }

                Item { Layout.fillWidth: true }

                // Connection status badge
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
                            text: (mqttClient && mqttClient.isConnected) ? "CONNECTED" : "DISCONNECTED"
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
                            if (mqttClient.clientId === "" || mqttClient.channelId === "") {
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

        // Center Work Area (LED Indicator + Control Buttons)
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

                // Left Column: LED Status Display
                Rectangle {
                    Layout.preferredWidth: 260
                    Layout.fillHeight: true
                    color: "#161D27"
                    radius: 10
                    border.color: root.cardBorder

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 16

                        Text {
                            text: "ESP32 HARDWARE STATE"
                            font.pixelSize: 12
                            font.bold: true
                            color: root.textSecondary
                            Layout.alignment: Qt.AlignHCenter
                        }

                        // Big Glowing LED Bulb
                        Rectangle {
                            id: ledBulb
                            width: 110
                            height: 110
                            radius: 55
                            Layout.alignment: Qt.AlignHCenter

                            readonly property bool isLedOn: mqttClient && mqttClient.ledState === 1
                            readonly property bool isLedOff: mqttClient && mqttClient.ledState === 0

                            color: isLedOn ? "#00E676" : (isLedOff ? "#202A38" : "#374151")
                            border.color: isLedOn ? "#69F0AE" : "#3B4758"
                            border.width: 3

                            // Glow effect
                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width + 24
                                height: parent.height + 24
                                radius: width / 2
                                color: "transparent"
                                border.color: ledBulb.isLedOn ? "#3300E676" : "transparent"
                                border.width: 10
                                visible: ledBulb.isLedOn
                            }

                            Text {
                                anchors.centerIn: parent
                                text: ledBulb.isLedOn ? "ON" : (ledBulb.isLedOff ? "OFF" : "?")
                                font.pixelSize: 26
                                font.bold: true
                                color: ledBulb.isLedOn ? "#042211" : root.textSecondary
                            }
                        }

                        Text {
                            text: (mqttClient && mqttClient.ledState === 1) ? "GPIO 2: HIGH (Active)" :
                                  ((mqttClient && mqttClient.ledState === 0) ? "GPIO 2: LOW (Inactive)" : "Waiting for state...")
                            font.pixelSize: 13
                            font.bold: true
                            color: (mqttClient && mqttClient.ledState === 1) ? root.accentGreen :
                                   ((mqttClient && mqttClient.ledState === 0) ? root.textSecondary : root.accentBlue)
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Text {
                            text: "Channel: " + (mqttClient && mqttClient.channelId ? mqttClient.channelId : "<Not Set>")
                            font.pixelSize: 11
                            color: "#68768A"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Right Column: Dual Buttons (ON / OFF) & Cooldown
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 18

                    Text {
                        text: "LED CONTROL COMMANDS"
                        font.pixelSize: 13
                        font.bold: true
                        color: root.textSecondary
                    }

                    // 2 Big Buttons Row
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        spacing: 20

                        // BUTTON 1: TURN ON
                        Button {
                            id: btnOn
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            enabled: mqttClient && mqttClient.isConnected && mqttClient.cooldownSeconds === 0

                            onClicked: {
                                if (mqttClient) mqttClient.turnOnLed()
                            }

                            contentItem: ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 6

                                Text {
                                    text: "⚡ TURN ON"
                                    font.pixelSize: 20
                                    font.bold: true
                                    color: btnOn.enabled ? "#FFFFFF" : "#5A6578"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Text {
                                    text: "Publish: field1 = 1"
                                    font.pixelSize: 12
                                    color: btnOn.enabled ? "#B9F6CA" : "#4A5568"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }

                            background: Rectangle {
                                radius: 10
                                color: btnOn.down ? "#00A854" : (btnOn.enabled ? "#00B359" : "#1B222E")
                                border.color: btnOn.enabled ? root.accentGreen : "#2E3846"
                                border.width: btnOn.enabled ? 2 : 1

                                Behavior on color { ColorAnimation { duration: 150 } }
                            }
                        }

                        // BUTTON 2: TURN OFF
                        Button {
                            id: btnOff
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            enabled: mqttClient && mqttClient.isConnected && mqttClient.cooldownSeconds === 0

                            onClicked: {
                                if (mqttClient) mqttClient.turnOffLed()
                            }

                            contentItem: ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 6

                                Text {
                                    text: "⏹ TURN OFF"
                                    font.pixelSize: 20
                                    font.bold: true
                                    color: btnOff.enabled ? "#FFFFFF" : "#5A6578"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Text {
                                    text: "Publish: field1 = 0"
                                    font.pixelSize: 12
                                    color: btnOff.enabled ? "#FFB4C4" : "#4A5568"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }

                            background: Rectangle {
                                radius: 10
                                color: btnOff.down ? "#C41C48" : (btnOff.enabled ? "#D81B4E" : "#1B222E")
                                border.color: btnOff.enabled ? root.accentRed : "#2E3846"
                                border.width: btnOff.enabled ? 2 : 1

                                Behavior on color { ColorAnimation { duration: 150 } }
                            }
                        }
                    }

                    // Cooldown Indicator (ThingSpeak 15s Rate Limit Notice)
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        radius: 8
                        color: (mqttClient && mqttClient.cooldownSeconds > 0) ? "#2B2115" : "#161D27"
                        border.color: (mqttClient && mqttClient.cooldownSeconds > 0) ? "#FF9800" : root.cardBorder

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 12

                            Text {
                                text: (mqttClient && mqttClient.cooldownSeconds > 0) ? "⏳" : "✓"
                                font.pixelSize: 16
                            }

                            Text {
                                text: (mqttClient && mqttClient.cooldownSeconds > 0)
                                      ? ("ThingSpeak Rate Limiting active: Cooldown " + mqttClient.cooldownSeconds + "s remaining")
                                      : "Ready to transmit commands (ThingSpeak Cloud MQTT 3.1.1)"
                                color: (mqttClient && mqttClient.cooldownSeconds > 0) ? "#FFB74D" : root.textSecondary
                                font.pixelSize: 12
                                Layout.fillWidth: true
                            }

                            ProgressBar {
                                Layout.preferredWidth: 120
                                Layout.preferredHeight: 8
                                visible: mqttClient && mqttClient.cooldownSeconds > 0
                                from: 0
                                to: 15
                                value: mqttClient ? (15 - mqttClient.cooldownSeconds) : 0
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // Bottom Status Bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
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
                    text: mqttClient ? mqttClient.statusMessage : ""
                    font.pixelSize: 12
                    color: (mqttClient && mqttClient.statusMessage.indexOf("Error") !== -1 ||
                            mqttClient && mqttClient.statusMessage.indexOf("closed") !== -1) ? root.accentOrange : root.textPrimary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Text {
                    text: "Target: ESP32 Built-in LED"
                    font.pixelSize: 11
                    color: root.accentBlue
                }
            }
        }
    }

    // Settings Dialog
    Dialog {
        id: settingsDialog
        title: "ThingSpeak MQTT Configuration"
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 600)
        standardButtons: Dialog.Save | Dialog.Cancel

        onAccepted: {
            if (!mqttClient) return
            mqttClient.host = txtHost.text.trim()
            mqttClient.port = parseInt(txtPort.text.trim()) || 1883
            mqttClient.channelId = txtChannelId.text.trim()
            mqttClient.clientId = txtClientId.text.trim()
            mqttClient.username = txtUsername.text.trim()
            mqttClient.password = txtPassword.text.trim()
            mqttClient.writeApiKey = txtWriteApiKey.text.trim()
            mqttClient.subscribeFeedback = chkFeedback.checked
            mqttClient.autoReconnect = chkAutoReconnect.checked
            mqttClient.saveSettings()
        }

        background: Rectangle {
            color: "#18202C"
            radius: 12
            border.color: root.cardBorder
            border.width: 2
        }

        contentItem: ColumnLayout {
            spacing: 10

            // Crucial Warning Banner regarding MQTT Client IDs
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: 6
                color: "#2C1B14"
                border.color: "#FF9800"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8
                    Text { text: "⚠️"; font.pixelSize: 18 }
                    Text {
                        text: "IMPORTANT: Do NOT use the ESP32's Client ID here! In ThingSpeak (Devices -> MQTT), create a 2nd device for this HMI. Sharing the same Client ID kicks both devices offline."
                        font.pixelSize: 11
                        color: "#FFCC80"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            GridLayout {
                columns: 2
                rowSpacing: 8
                columnSpacing: 12
                Layout.fillWidth: true

                Text { text: "Broker Host:"; color: root.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: txtHost
                    Layout.fillWidth: true
                    text: mqttClient ? mqttClient.host : "mqtt3.thingspeak.com"
                    placeholderText: "mqtt3.thingspeak.com"
                    color: "#FFFFFF"
                }

                Text { text: "Broker Port:"; color: root.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: txtPort
                    Layout.fillWidth: true
                    text: mqttClient ? mqttClient.port.toString() : "1883"
                    placeholderText: "1883 (plain) or 8883 (TLS)"
                    color: "#FFFFFF"
                }

                Text { text: "Channel ID:"; color: root.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: txtChannelId
                    Layout.fillWidth: true
                    text: mqttClient ? mqttClient.channelId : ""
                    placeholderText: "e.g. 3564388"
                    color: "#FFFFFF"
                }

                Text { text: "HMI Client ID:"; color: root.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: txtClientId
                    Layout.fillWidth: true
                    text: mqttClient ? mqttClient.clientId : ""
                    placeholderText: "Device 2 Client ID (must differ from ESP32)"
                    color: "#FFFFFF"
                }

                Text { text: "HMI Username:"; color: root.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: txtUsername
                    Layout.fillWidth: true
                    text: mqttClient ? mqttClient.username : ""
                    placeholderText: "Device 2 Username"
                    color: "#FFFFFF"
                }

                Text { text: "HMI Password:"; color: root.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: txtPassword
                    Layout.fillWidth: true
                    text: mqttClient ? mqttClient.password : ""
                    placeholderText: "Device 2 Password"
                    echoMode: TextInput.Password
                    color: "#FFFFFF"
                }

                Text { text: "Write API Key:"; color: root.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: txtWriteApiKey
                    Layout.fillWidth: true
                    text: mqttClient ? mqttClient.writeApiKey : ""
                    placeholderText: "Channel Write API Key (Optional fallback)"
                    echoMode: TextInput.Password
                    color: "#FFFFFF"
                }
            }

            RowLayout {
                spacing: 16

                CheckBox {
                    id: chkFeedback
                    text: "Subscribe for live state confirmation"
                    checked: mqttClient ? mqttClient.subscribeFeedback : false
                    contentItem: Text {
                        text: chkFeedback.text
                        color: root.textSecondary
                        font.pixelSize: 11
                        leftPadding: chkFeedback.indicator.width + 6
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                CheckBox {
                    id: chkAutoReconnect
                    text: "Auto-reconnect on drop"
                    checked: mqttClient ? mqttClient.autoReconnect : true
                    contentItem: Text {
                        text: chkAutoReconnect.text
                        color: root.textSecondary
                        font.pixelSize: 11
                        leftPadding: chkAutoReconnect.indicator.width + 6
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }
}
