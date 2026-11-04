/*
  ====================================================================================
  ESP32 ThingSpeak MQTT Built-in LED Controller
  ====================================================================================
  Description:
    Connects to ThingSpeak MQTT Broker (mqtt3.thingspeak.com) over Wi-Fi and listens
    for LED ON/OFF commands sent by the Raspberry Pi CM5 Lite QML/C++ HMI.
    
    Subscribed Topic:
      channels/<CHANNEL_ID>/subscribe/fields/field1
      
    Payloads handled:
      "1" -> Turn Built-in LED ON
      "0" -> Turn Built-in LED OFF

  Hardware:
    - ESP32 Development Board (ESP-WROOM-32 / NodeMCU / DevKit v1)
    - Built-in LED: Typically GPIO 2 (Active HIGH)
      * Note: If your ESP32 board uses a different pin (e.g. GPIO 8 on ESP32-C3)
              or active-LOW logic, adjust LED_PIN and LED_ON/LED_OFF below.

  Required Arduino Libraries:
    - WiFi (built-in for ESP32 core)
    - PubSubClient (by Nick O'Leary) - Install via Arduino Library Manager
  ====================================================================================
*/

#include <WiFi.h>
#include <PubSubClient.h>

// ----------------------------------------------------------------------------------
// 1. Wi-Fi Configuration (2.4 GHz only; ESP32 does not support 5 GHz)
// ----------------------------------------------------------------------------------
const char* WIFI_SSID     = "Room_302_40040A";
const char* WIFI_PASSWORD = "Room@302";

// ----------------------------------------------------------------------------------
// 2. ThingSpeak MQTT Broker Configuration
//    Obtain these credentials from ThingSpeak:
//    - Go to: https://thingspeak.mathworks.com/
//    - Go to: Devices -> MQTT Devices -> Add a new device
//    - Authorize your Channel and copy the generated credentials
// ----------------------------------------------------------------------------------
const char* MQTT_BROKER   = "mqtt3.thingspeak.com";
const int   MQTT_PORT     = 1883;

// Channel ID (Numeric ID of your ThingSpeak channel, e.g. "2458123")
const char* CHANNEL_ID    = "3564388";

// ESP32 MQTT Device Credentials (MUST be unique from the Raspberry Pi HMI credentials)
const char* MQTT_CLIENT_ID = "KwcdGSgBFBoJOQQ9CB43KyY";
const char* MQTT_USERNAME  = "KwcdGSgBFBoJOQQ9CB43KyY";
const char* MQTT_PASSWORD  = "tQGJJHihRKZFdSpo11tjjx34";

// ----------------------------------------------------------------------------------
// 3. Hardware Pinout
// ----------------------------------------------------------------------------------
#define LED_PIN 2       // Default built-in LED for ESP32 DevKit / NodeMCU
#define LED_ON  HIGH    // Active HIGH (set to LOW if your board's LED is inverted)
#define LED_OFF LOW     // Inactive LOW

// MQTT Topics
String subscribeTopic = "channels/" + String(CHANNEL_ID) + "/subscribe/fields/field1";
String publishTopic   = "channels/" + String(CHANNEL_ID) + "/publish";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastReconnectAttempt = 0;

// ----------------------------------------------------------------------------------
// Wi-Fi Connection Routine
// ----------------------------------------------------------------------------------
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("[WiFi] Connecting to: ");
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
      Serial.println("\n[WiFi] Connection failed! Restarting ESP32...");
      ESP.restart();
    }
  }

  digitalWrite(LED_PIN, LED_OFF);
  Serial.println("\n[WiFi] Connected successfully!");
  Serial.print("[WiFi] IP Address: ");
  Serial.println(WiFi.localIP());
}

// ----------------------------------------------------------------------------------
// MQTT Incoming Message Callback
// ----------------------------------------------------------------------------------
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  // Convert payload to null-terminated C-string
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.println();
  Serial.println("========================================");
  Serial.print("[MQTT] Message arrived on topic: ");
  Serial.println(topic);
  Serial.print("[MQTT] Payload content: ");
  Serial.println(message);

  String payloadStr = String(message);
  payloadStr.trim();

  // Control Built-in LED based on received command
  if (payloadStr == "1" || payloadStr.equalsIgnoreCase("ON") || payloadStr.indexOf("field1=1") >= 0) {
    digitalWrite(LED_PIN, LED_ON);
    Serial.println("[LED] State -> ON (GPIO " + String(LED_PIN) + " set HIGH)");
  } else if (payloadStr == "0" || payloadStr.equalsIgnoreCase("OFF") || payloadStr.indexOf("field1=0") >= 0) {
    digitalWrite(LED_PIN, LED_OFF);
    Serial.println("[LED] State -> OFF (GPIO " + String(LED_PIN) + " set LOW)");
  } else {
    Serial.println("[LED] Unrecognized command payload: '" + payloadStr + "'");
  }
  Serial.println("========================================");
}

// ----------------------------------------------------------------------------------
// MQTT Connection & Subscription Routine
// ----------------------------------------------------------------------------------
boolean reconnectMQTT() {
  Serial.print("[MQTT] Connecting to broker ");
  Serial.print(MQTT_BROKER);
  Serial.print(" as ClientID: ");
  Serial.println(MQTT_CLIENT_ID);

  // Attempt connection with credentials
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("[MQTT] Connected to ThingSpeak Broker!");

    // Subscribe to the channel's field1 topic
    subscribeTopic = "channels/" + String(CHANNEL_ID) + "/subscribe/fields/field1";
    Serial.print("[MQTT] Subscribing to: ");
    Serial.println(subscribeTopic);

    if (mqttClient.subscribe(subscribeTopic.c_str())) {
      Serial.println("[MQTT] Successfully subscribed! Waiting for commands from RPi HMI...");
      // Double quick blink to confirm connection
      for (int i = 0; i < 2; i++) {
        digitalWrite(LED_PIN, LED_ON);
        delay(100);
        digitalWrite(LED_PIN, LED_OFF);
        delay(100);
      }
    } else {
      Serial.println("[MQTT] Subscription failed!");
    }
    return true;
  } else {
    Serial.print("[MQTT] Connect failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" (Will retry in 5 seconds)");
    return false;
  }
}

// ----------------------------------------------------------------------------------
// Setup
// ----------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- ESP32 ThingSpeak MQTT Receiver Initializing ---");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  setupWiFi();

  // Configure MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(512); // Buffer size for MQTT packets
}

// ----------------------------------------------------------------------------------
// Main Loop
// ----------------------------------------------------------------------------------
void loop() {
  // Ensure Wi-Fi remains connected
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
  }

  // Ensure MQTT client remains connected
  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnectMQTT()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Process incoming MQTT messages and maintain keep-alive ping
    mqttClient.loop();
  }
}

