/*
  ====================================================================================
  ESP32 Mosquitto MQTT Built-in LED Controller
  ====================================================================================
  Description:
    Connects to a local Mosquitto MQTT broker over Wi-Fi and listens for LED ON/OFF
    commands published by the Raspberry Pi CM5 Lite QML/C++ HMI.
    
    Subscribed Topic:
      esp32/led/set
      
    Payloads Handled:
      "ON"  or "1" -> Turns Built-in LED ON, publishes "ON" to esp32/led/status
      "OFF" or "0" -> Turns Built-in LED OFF, publishes "OFF" to esp32/led/status

    Published Topics:
      esp32/led/status -> "ON" or "OFF" (Retained: true, so HMI gets instant state)
      esp32/status     -> "online" / "offline" (LWT - Last Will & Testament)

  Hardware:
    - ESP32 Development Board (ESP-WROOM-32 / NodeMCU / DevKit v1)
    - Connected to Ubuntu PC via USB cable (for power & serial monitor at 115200 baud)
    - Built-in LED: Pin 2 (GPIO 2, Active HIGH on standard ESP32 DevKit)

  Required Libraries (Arduino Library Manager or PlatformIO):
    - WiFi (built into ESP32 Arduino Core)
    - PubSubClient (by Nick O'Leary)
  ====================================================================================
*/

#include <WiFi.h>
#include <PubSubClient.h>

// ==================================================================================
// 1. Wi-Fi Configuration (2.4 GHz network)
// ==================================================================================
const char* WIFI_SSID     = "YOUR_WIFI_SSID";       // <--- Replace with your Wi-Fi SSID
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";   // <--- Replace with your Wi-Fi Password

// ==================================================================================
// 2. Mosquitto MQTT Broker Configuration (Ubuntu PC or Raspberry Pi IP)
// ==================================================================================
// Set this to your Ubuntu PC's LAN IP (run `hostname -I` or `ip a` on Ubuntu)
// e.g., "192.168.1.50"
const char* MQTT_BROKER   = "192.168.1.100";        // <--- Replace with Ubuntu PC IP
const int   MQTT_PORT     = 1883;                   // Default Mosquitto port

// Unique Client ID for ESP32 (MUST be different from Raspberry Pi HMI Client ID)
const char* MQTT_CLIENT_ID = "esp32_builtin_led";

// Optional credentials (leave empty "" if allow_anonymous is true in mosquitto.conf)
const char* MQTT_USERNAME  = "";
const char* MQTT_PASSWORD  = "";

// MQTT Topics
const char* TOPIC_LED_CMD    = "esp32/led/set";     // Receive commands (ON / OFF)
const char* TOPIC_LED_STATUS = "esp32/led/status";  // Publish LED status feedback
const char* TOPIC_DEV_STATUS = "esp32/status";      // Device online / offline (LWT)

// ==================================================================================
// 3. Hardware Pinout
// ==================================================================================
#define LED_PIN  2     // Default built-in LED for ESP32 DevKit (GPIO 2)
#define LED_ON   HIGH  // Active HIGH logic (set to LOW if using inverted board)
#define LED_OFF  LOW

// ==================================================================================
// Globals
// ==================================================================================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastReconnectAttempt = 0;
bool currentLedState = false;

// ==================================================================================
// MQTT Incoming Message Callback
// ==================================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert payload byte array to String
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  Serial.println();
  Serial.print("[MQTT RX] Topic: ");
  Serial.print(topic);
  Serial.print(" | Payload: ");
  Serial.println(message);

  if (String(topic) == TOPIC_LED_CMD) {
    if (message.equalsIgnoreCase("ON") || message == "1" || message.equalsIgnoreCase("TRUE")) {
      digitalWrite(LED_PIN, LED_ON);
      currentLedState = true;
      Serial.println("[LED] Status changed to: ON (GPIO 2 HIGH)");

      // Publish state confirmation with retain=true so any newly connected HMI knows
      mqttClient.publish(TOPIC_LED_STATUS, "ON", true);
    } 
    else if (message.equalsIgnoreCase("OFF") || message == "0" || message.equalsIgnoreCase("FALSE")) {
      digitalWrite(LED_PIN, LED_OFF);
      currentLedState = false;
      Serial.println("[LED] Status changed to: OFF (GPIO 2 LOW)");

      // Publish state confirmation with retain=true
      mqttClient.publish(TOPIC_LED_STATUS, "OFF", true);
    } 
    else {
      Serial.print("[WARN] Unknown command received: ");
      Serial.println(message);
    }
  }
}

// ==================================================================================
// Wi-Fi Connection
// ==================================================================================
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.println("==================================================");
  Serial.print("[WiFi] Connecting to SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Blink LED slowly while searching for Wi-Fi
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    attempts++;
    if (attempts > 60) { // 30 seconds timeout
      Serial.println("\n[WiFi] Connection timeout. Retrying...");
      attempts = 0;
    }
  }

  // Turn LED OFF once connected to Wi-Fi
  digitalWrite(LED_PIN, LED_OFF);
  currentLedState = false;

  Serial.println("\n[WiFi] Connected successfully!");
  Serial.print("[WiFi] ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("[WiFi] Signal RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.println("==================================================");
}

// ==================================================================================
// Non-blocking Mosquitto Broker Connection
// ==================================================================================
bool reconnectMqtt() {
  Serial.print("[MQTT] Connecting to Mosquitto Broker at ");
  Serial.print(MQTT_BROKER);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.print(" as '");
  Serial.print(MQTT_CLIENT_ID);
  Serial.print("' ... ");

  // Connect with Last Will and Testament (LWT)
  // If ESP32 abruptly loses power, Mosquitto automatically publishes "offline" to esp32/status
  bool connected = false;
  if (strlen(MQTT_USERNAME) > 0) {
    connected = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD,
                                   TOPIC_DEV_STATUS, 1, true, "offline");
  } else {
    connected = mqttClient.connect(MQTT_CLIENT_ID,
                                   TOPIC_DEV_STATUS, 1, true, "offline");
  }

  if (connected) {
    Serial.println("CONNECTED!");

    // 1. Publish birth message (device online)
    mqttClient.publish(TOPIC_DEV_STATUS, "online", true);

    // 2. Publish initial LED state
    mqttClient.publish(TOPIC_LED_STATUS, currentLedState ? "ON" : "OFF", true);

    // 3. Subscribe to command topic
    mqttClient.subscribe(TOPIC_LED_CMD);
    Serial.print("[MQTT] Subscribed to command topic: ");
    Serial.println(TOPIC_LED_CMD);
    return true;
  } else {
    Serial.print("FAILED (rc=");
    Serial.print(mqttClient.state());
    Serial.println(")");
    return false;
  }
}

// ==================================================================================
// Setup
// ==================================================================================
void setup() {
  // Initialize Serial Monitor (Ubuntu PC connection via USB)
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n--- ESP32 Mosquitto MQTT LED Controller Initializing ---");

  // Configure Built-in LED GPIO
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  // Connect to Wi-Fi
  setupWiFi();

  // Configure MQTT client
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(30);
}

// ==================================================================================
// Main Loop
// ==================================================================================
void loop() {
  // Ensure Wi-Fi is still alive
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Lost connection. Reconnecting...");
    setupWiFi();
  }

  // Ensure MQTT client is connected (non-blocking retry every 5 seconds)
  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnectMqtt()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Process incoming MQTT packets
    mqttClient.loop();
  }
}
