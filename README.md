# Mosquitto MQTT Built-in LED Controller: Raspberry Pi CM5 Lite & ESP32

Complete end-to-end IoT solution to control the built-in LED of an **ESP32** from a **Qt Quick (QML) and modern C++ Human Machine Interface (HMI)** running on a **Raspberry Pi Compute Module 5 Lite (2GB RAM, Raspberry Pi OS Lite CLI)**, communicating over **MQTT** via a local **Mosquitto** broker hosted on an **Ubuntu PC**.

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
                                    │ MQTT (Port 1883)
                                    │ Publish:  esp32/led/set   ("ON" / "OFF")
                                    │ Subscribe: esp32/led/status ("ON" / "OFF")
                                    ▼
         ┌──────────────────────────────────────────────────────┐
         │              Ubuntu PC (Local LAN)                   │
         │  - Mosquitto MQTT Broker (port 1883)                 │
         │  - Connected to ESP32 via USB (Flash & Serial Mon)   │
         └──────────────────────────┬───────────────────────────┘
                                    │ MQTT over Wi-Fi (Port 1883)
                                    │ Subscribe: esp32/led/set
                                    │ Publish:   esp32/led/status
                                    ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                         ESP32 NodeMCU / DevKit                         │
 │  ┌──────────────────────────────────────────────────────────────────┐  │
 │  │ WiFiClient + PubSubClient Listener                               │  │
 │  │                                                                  │  │
 │  │   "ON"  ──► digitalWrite(LED_PIN, HIGH) ──► [💡 Built-in LED ON]  │  │
 │  │   "OFF" ──► digitalWrite(LED_PIN, LOW)  ──► [⭕ Built-in LED OFF] │  │
 │  └──────────────────────────────────────────────────────────────────┘  │
 │  Connected via USB to Ubuntu PC (for power & 115200 baud monitor)      │
 └────────────────────────────────────────────────────────────────────────┘
```

---

## 📁 Repository Structure

```text
mqtt_esp/
├── mosquitto/
│   ├── mosquitto.conf                  # Mosquitto broker configuration (0.0.0.0 bind)
│   └── start_broker.sh                 # Launch script for broker on Ubuntu PC
├── esp32_firmware/
│   ├── esp32_mqtt_mosquitto/
│   │   └── esp32_mqtt_mosquitto.ino    # Arduino sketch for ESP32
│   ├── platformio.ini                  # PlatformIO project config (VSCode on Ubuntu)
│   └── src/
│       └── main.cpp                    # PlatformIO entry point
├── rpi_hmi/
│   ├── CMakeLists.txt                  # Modern CMake build system (Qt 6 & Qt 5 compatible)
│   ├── resources.qrc                   # Qt Resource bundle
│   ├── src/
│   │   ├── main.cpp                    # App entry point & QML engine setup
│   │   ├── mqttclient.h                # Standalone Qt C++ MQTT 3.1.1 client header
│   │   └── mqttclient.cpp              # Protocol implementation over QTcpSocket
│   └── qml/
│       └── Main.qml                    # Industrial touch HMI with 2 buttons & live status
├── scripts/
│   ├── setup_rpi_lite.sh               # Auto-setup script for Raspberry Pi OS Lite
│   ├── run_hmi.sh                      # Launch script with EGLFS / LinuxFB KMS detection
│   └── rpi-hmi.service                 # systemd service for auto-start on boot
└── README.md
```

---

## 🚀 Quick Start Guide

### Step 1: Configure & Start Mosquitto on Ubuntu PC

Ubuntu PC serves as the central MQTT Broker. By default, Mosquitto 2.0+ binds only to `localhost` (`127.0.0.1`). We provide a configuration file that opens port `1883` to local network devices (ESP32 and Raspberry Pi).

1. **Install Mosquitto**:
   ```bash
   sudo apt update
   sudo apt install -y mosquitto mosquitto-clients
   ```

2. **Check your Ubuntu PC's IP Address**:
   ```bash
   hostname -I
   ```
   *(e.g., `192.168.1.100` — note this IP down, as both ESP32 and CM5 will connect to it!)*

3. **Allow MQTT in Firewall (if UFW is active)**:
   ```bash
   sudo ufw allow 1883/tcp
   ```

4. **Start the Broker**:
   Run the included startup script:
   ```bash
   chmod +x mosquitto/start_broker.sh
   ./mosquitto/start_broker.sh
   ```
   *Alternatively, configure systemd mosquitto by copying the config:*
   ```bash
   sudo cp mosquitto/mosquitto.conf /etc/mosquitto/conf.d/local.conf
   sudo systemctl restart mosquitto
   ```

---

### Step 2: Flash ESP32 Firmware

The ESP32 is plugged into the Ubuntu PC via USB for flashing and power, and connects to the Mosquitto broker via Wi-Fi.

#### Hardware Pinout
- **Built-in LED**: GPIO 2 (Active HIGH on standard ESP-WROOM-32 / DevKit v1).

#### Option A: Flashing via Arduino IDE
1. Open Arduino IDE on Ubuntu.
2. Install the **ESP32** board package in Boards Manager (`esp32` by Espressif Systems).
3. Install **PubSubClient** library via Library Manager (`PubSubClient` by Nick O'Leary).
4. Open `esp32_firmware/esp32_mqtt_mosquitto/esp32_mqtt_mosquitto.ino`.
5. Update your Wi-Fi credentials and Ubuntu PC IP:
   ```cpp
   const char* WIFI_SSID     = "Your_WiFi_Name";
   const char* WIFI_PASSWORD = "Your_WiFi_Password";
   const char* MQTT_BROKER   = "192.168.1.100"; // Ubuntu PC IP
   ```
6. Select your Board (**ESP32 Dev Module**) and Port (e.g., `/dev/ttyUSB0`).
7. Click **Upload**, then open the **Serial Monitor** at **115200 baud** to confirm:
   ```text
   [WiFi] Connected successfully! IP: 192.168.1.105
   [MQTT] Connecting to Mosquitto Broker at 192.168.1.100:1883 ... CONNECTED!
   [MQTT] Subscribed to command topic: esp32/led/set
   ```

#### Option B: Flashing via PlatformIO (VSCode)
1. Open the repository root in VSCode.
2. Edit `esp32_firmware/esp32_mqtt_mosquitto/esp32_mqtt_mosquitto.ino` with your Wi-Fi and PC IP.
3. In PlatformIO toolbar, click **Build** and **Upload**.
4. Open the Serial Monitor (`pio device monitor -b 115200`).

---

### Step 3: Setup & Run HMI on Raspberry Pi CM5 Lite

Raspberry Pi OS Lite runs without an X11/Wayland desktop. The Qt HMI is designed to render directly to the screen via **KMS/DRM** and **EGLFS** (hardware-accelerated OpenGL fullscreen) with fallback to **LinuxFB**.

#### 3.1 Install Dependencies & Build
On your Raspberry Pi CM5 Lite terminal:
```bash
# Clone the repository onto your Pi (or copy it over scp/sftp)
git clone <your-repo-url> ~/mqtt_esp
cd ~/mqtt_esp

# Make setup script executable and run it
chmod +x scripts/setup_rpi_lite.sh
./scripts/setup_rpi_lite.sh
```

> **Important:** After running the script, log out and log back in (or `sudo reboot`) so the `video`, `render`, and `input` group permissions take effect for direct console display and touch input.

#### 3.2 Launch the HMI
```bash
./scripts/run_hmi.sh
```

#### 3.3 Configure Broker IP in the HMI
1. When the HMI appears on the screen, tap or click **⚙ Settings** (top-right).
2. Enter the **Mosquitto Broker Host / IP** (e.g. `192.168.1.100`).
3. Port: `1883`.
4. Click **Save & Connect**.
5. The badge will switch to green **BROKER CONNECTED**, and the **ESP32: ONLINE** indicator will activate.

#### 3.4 Auto-Start HMI on Boot (systemd service)
To run the HMI automatically every time the CM5 boots:
```bash
sudo cp scripts/rpi-hmi.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable rpi-hmi.service
sudo systemctl start rpi-hmi.service
```

---

## 🧪 Testing with Command-Line Tools

You can verify the entire MQTT pipeline from any terminal on Ubuntu or Raspberry Pi:

### Listen to LED Status Feedback:
```bash
mosquitto_sub -h localhost -t "esp32/#" -v
```

### Turn ON the ESP32 LED via CLI:
```bash
mosquitto_pub -h localhost -t "esp32/led/set" -m "ON"
```
*(ESP32 will light up its blue LED and publish `"esp32/led/status ON"`)*

### Turn OFF the ESP32 LED via CLI:
```bash
mosquitto_pub -h localhost -t "esp32/led/set" -m "OFF"
```
*(ESP32 will extinguish its blue LED and publish `"esp32/led/status OFF"`)*

---

## 🛠️ Troubleshooting & Tips

| Symptom | Cause | Solution |
| :--- | :--- | :--- |
| **HMI says "Connection refused"** | Mosquitto is only listening on localhost | Ensure `mosquitto/mosquitto.conf` has `listener 1883 0.0.0.0` and `allow_anonymous true`. |
| **ESP32 keeps printing rc=-2** | ESP32 cannot reach Ubuntu PC IP | Ensure ESP32 and PC are on the same Wi-Fi subnet (2.4 GHz). Check Ubuntu firewall (`sudo ufw allow 1883/tcp`). |
| **HMI displays black screen on CM5** | DRM/KMS permissions issue on OS Lite | Run `sudo usermod -a -G video,render,input $USER` and reboot. Fallback to `linuxfb` via `./scripts/run_hmi.sh`. |
| **ESP32 LED logic inverted** | Some boards use active-LOW LEDs | In `esp32_mqtt_mosquitto.ino`, change `#define LED_ON LOW` and `#define LED_OFF HIGH`. |
| **Multiple clients disconnecting** | Reusing the same MQTT `ClientID` | The HMI uses `rpi_cm5_hmi_<id>` and ESP32 uses `esp32_builtin_led`. Ensure both have distinct IDs. |
