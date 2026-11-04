# ThingSpeak MQTT Built-in LED Controller: Raspberry Pi CM5 Lite & ESP32

Complete end-to-end IoT solution to control the built-in LED of an **ESP32** from a **Qt Quick (QML) and modern C++ Human Machine Interface (HMI)** running on a **Raspberry Pi Compute Module 5 Lite (2GB RAM, Raspberry Pi OS Lite CLI)**, communicating over **MQTT** via the **ThingSpeak Cloud**.

---

## 📐 System Architecture

```text
 ┌────────────────────────────────────────────────────────────────────────┐
 │                    Raspberry Pi CM5 Lite (CLI)                         │
 │                                                                        │
 │  ┌──────────────────────────────────────────────────────────────────┐  │
 │  │                QML & C++ Embedded HMI (EGLFS / DRM)              │  │
 │  │      [⚡ TURN ON LED]                   [⏹ TURN OFF LED]         │  │
 │  └───────────────────────────────┬──────────────────────────────────┘  │
 └──────────────────────────────────┼─────────────────────────────────────┘
                                    │ MQTT Publish (QoS 0)
                                    │ Topic: channels/<CHANNEL_ID>/publish
                                    │ Payload: "field1=1" or "field1=0"
                                    ▼
             ┌──────────────────────────────────────────────┐
             │            ThingSpeak MQTT Cloud             │
             │           (mqtt3.thingspeak.com)             │
             └──────────────────────┬───────────────────────┘
                                    │ MQTT Subscribe
                                    │ Topic: channels/<CHANNEL_ID>/subscribe/fields/field1
                                    │ Payload: "1" or "0"
                                    ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                         ESP32 NodeMCU / DevKit                         │
 │  ┌──────────────────────────────────────────────────────────────────┐  │
 │  │ WiFiClient + PubSubClient Listener                               │  │
 │  │                                                                  │  │
 │  │   "1" ──► digitalWrite(LED_PIN, HIGH) ──► [💡 Built-in LED ON]   │  │
 │  │   "0" ──► digitalWrite(LED_PIN, LOW)  ──► [⭕ Built-in LED OFF]  │  │
 │  └──────────────────────────────────────────────────────────────────┘  │
 │  Connected via USB to Ubuntu PC (for flashing & 115200 baud monitor)    │
 └────────────────────────────────────────────────────────────────────────┘
```

---

## 📁 Repository Structure

```text
mqtt_esp/
├── esp32_firmware/
│   ├── esp32_mqtt_thingspeak/
│   │   └── esp32_mqtt_thingspeak.ino   # Arduino sketch for ESP32
│   ├── platformio.ini                  # PlatformIO project config (for VSCode on Ubuntu)
│   └── src/
│       └── main.cpp                    # PlatformIO entry point
├── rpi_hmi/
│   ├── CMakeLists.txt                  # Modern CMake build system (Qt 6 & Qt 5 compatible)
│   ├── resources.qrc                   # Qt Resource bundle
│   ├── src/
│   │   ├── main.cpp                    # App entry point & QML engine setup
│   │   ├── mqttclient.h                # Standalone Qt C++ MQTT 3.1.1 client header
│   │   └── mqttclient.cpp              # Protocol implementation over QTcpSocket/QSslSocket
│   └── qml/
│       └── Main.qml                    # Industrial touch HMI with 2 buttons & status
├── scripts/
│   ├── setup_rpi_lite.sh               # Auto-setup script for Raspberry Pi OS Lite
│   ├── run_hmi.sh                      # Launch script with EGLFS / LinuxFB KMS detection
│   └── rpi-hmi.service                 # systemd service for auto-start on boot
└── README.md
```

---

## ☁️ Step 1: ThingSpeak Cloud Setup

ThingSpeak provides a free MQTT 3.1.1 broker at `mqtt3.thingspeak.com`.

### 1.1 Create a Channel
1. Log in to [ThingSpeak](https://thingspeak.mathworks.com/).
2. Navigate to **Channels** -> **My Channels** -> **New Channel**.
3. Name: `ESP32 LED Controller`
4. Enable **Field 1** and name it: `LED_State`
5. Click **Save Channel**. Note down your numeric **Channel ID** (e.g. `2458901`).

### 1.2 Create MQTT Devices (Crucial Step!)
> **Important Note on MQTT:** The MQTT protocol prohibits two active connections sharing the same `ClientID`. If the Raspberry Pi and ESP32 attempt to connect using the same Client ID, the broker will continuously disconnect them. You should create **two** MQTT Devices authorized on your channel:

1. In ThingSpeak, go to **Devices** -> **MQTT Devices**.
2. Click **Add a new device**:
   - **Device 1 (For Raspberry Pi HMI):**
     - Name: `RPi_CM5_HMI`
     - Authorize Channel: Select your channel and grant **Write & Read** permission.
     - Click **Add Device**.
     - Download or copy the credentials:
       - `Client ID`
       - `Username`
       - `Password`
3. Click **Add a new device** again:
   - **Device 2 (For ESP32):**
     - Name: `ESP32_Device`
     - Authorize Channel: Select your channel and grant **Read** permission.
     - Click **Add Device**.
     - Copy the credentials:
       - `Client ID`
       - `Username`
       - `Password`

---

## ⚡ Step 2: ESP32 Firmware Setup (on Ubuntu PC)

Your ESP32 is plugged into the Ubuntu PC via USB.

### 2.1 Set USB Serial Permissions on Ubuntu
Add your Ubuntu user to the `dialout` group to allow flashing without root:
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in, or run 'newgrp dialout'
```

### 2.2 Configure Firmware Credentials
Open `esp32_firmware/esp32_mqtt_thingspeak/esp32_mqtt_thingspeak.ino` and update:
```cpp
// 1. Wi-Fi credentials (2.4 GHz only)
const char* WIFI_SSID     = "Your_WiFi_SSID";
const char* WIFI_PASSWORD = "Your_WiFi_Password";

// 2. ThingSpeak credentials
const char* CHANNEL_ID     = "YOUR_NUMERIC_CHANNEL_ID";
const char* MQTT_CLIENT_ID = "YOUR_ESP32_MQTT_CLIENT_ID";
const char* MQTT_USERNAME  = "YOUR_ESP32_MQTT_USERNAME";
const char* MQTT_PASSWORD  = "YOUR_ESP32_MQTT_PASSWORD";
```

### 2.3 Flash Using Arduino IDE
1. Open the Arduino IDE on Ubuntu.
2. Ensure ESP32 board support is installed (URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`).
3. Install the **PubSubClient** library via **Tools -> Manage Libraries...**.
4. Select your board: **ESP32 Dev Module** (or your specific ESP32 variant).
5. Select Port: `/dev/ttyUSB0` or `/dev/ttyACM0`.
6. Click **Upload**.
7. Open Serial Monitor at **115200 baud** to see Wi-Fi and MQTT connection logs:
   ```text
   --- ESP32 ThingSpeak MQTT Receiver Initializing ---
   [WiFi] Connected successfully! IP: 192.168.1.150
   [MQTT] Connected to ThingSpeak Broker!
   [MQTT] Subscribing to: channels/2458901/subscribe/fields/field1
   [MQTT] Successfully subscribed! Waiting for commands from RPi HMI...
   ```

*(Alternatively, if using PlatformIO in VSCode on Ubuntu, run `pio run -t upload -d esp32_firmware`)*

---

## 🍓 Step 3: Raspberry Pi CM5 Lite Setup & Build

The Raspberry Pi CM5 Lite runs **Raspberry Pi OS Lite (CLI)** without a bulky desktop environment. The HMI uses Qt Quick with the **EGLFS (KMS/DRM)** platform plugin to render directly to your connected display with full hardware acceleration.

### 3.1 Automatic Setup Script
Clone or copy the repository to your Raspberry Pi, then execute:
```bash
cd mqtt_esp/scripts
chmod +x *.sh
./setup_rpi_lite.sh
```
This script will:
- Install Qt6 development tools and QML modules (`qt6-base-dev`, `qt6-declarative-dev`, `qml6-module-qtquick-controls`, etc.).
- Add your user to the `video`, `render`, and `input` groups (required for KMS console graphics and touch/mouse input).
- Compile the HMI executable in `rpi_hmi/build/rpi_esp_hmi`.

### 3.2 Reboot or Re-login
For group permissions to take effect:
```bash
sudo reboot
```

### 3.3 Launch the HMI on Raspberry Pi OS Lite
From the console:
```bash
cd ~/mqtt_esp/scripts
./run_hmi.sh
```
The script automatically configures:
- `QT_QPA_PLATFORM=eglfs` (KMS/DRM hardware acceleration) with fallback to `linuxfb`.
- `QT_QPA_GENERIC_PLUGINS=evdevtouch,evdevmouse,evdevkeyboard` for input devices.

---

## 🖥️ Step 4: Operating the HMI

When the HMI appears on the screen:

1. Click **⚙ Settings**:
   - Host: `mqtt3.thingspeak.com`
   - Port: `1883`
   - Channel ID: Your numeric ThingSpeak Channel ID
   - MQTT Client ID: Device 1 Client ID (from ThingSpeak RPi Device)
   - MQTT Username: Device 1 Username
   - MQTT Password: Device 1 Password
   - Click **Save**. Settings are automatically saved to disk via `QSettings`!
2. Click **Connect**:
   - The status pill will change to **CONNECTED** (Green).
3. Control the ESP32:
   - Click **⚡ TURN ON**: Publishes `field1=1`. The ESP32's built-in LED (GPIO 2) turns ON instantly.
   - Click **⏹ TURN OFF**: Publishes `field1=0`. The ESP32's built-in LED turns OFF instantly.

> **💡 ThingSpeak Rate Limiting:**
> Free ThingSpeak accounts enforce a 15-second update interval per channel. The HMI includes an automatic cooldown timer that visually indicates the remaining seconds before the next command can be accepted by the cloud.

---

## 🔄 Optional: Auto-Start HMI on Boot (systemd)

To make your Raspberry Pi CM5 boot directly into the HMI like a dedicated appliance:

```bash
sudo cp ~/mqtt_esp/scripts/rpi-hmi.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable rpi-hmi.service
sudo systemctl start rpi-hmi.service
```

To stop or disable:
```bash
sudo systemctl stop rpi-hmi.service
sudo systemctl disable rpi-hmi.service
```

