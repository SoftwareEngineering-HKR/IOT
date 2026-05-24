#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "config.h" 


#define DHT_TYPE DHT11
DHT dhtSensor(DHT_PIN, DHT_TYPE);

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
WiFiUDP udp;

const int UDP_PORT = 4444; 
IPAddress serverIP;
bool serverFound = false;

const unsigned long UPDATE_INTERVAL = 60000; 
unsigned long lastUpdate = 0;

// Base MAC String
String baseMac;


void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  
  byte mac[6];
  WiFi.macAddress(mac);
  for (int i = 0; i < 6; ++i) {
    baseMac += String(mac[i], HEX);
  }
  baseMac.toUpperCase();
}

void discoverServer() {
  Serial.println("Broadcasting UDP to discover server...");
  udp.begin(UDP_PORT);
  
  while (!serverFound) {
    udp.beginPacket(IPAddress(255, 255, 255, 255), UDP_PORT);
    udp.print("NetworkDiscovery;Ver=1;"); 
    udp.endPacket();
    
    delay(2000);
    int packetSize = udp.parsePacket();
    if (packetSize) {
      serverIP = udp.remoteIP();
      serverFound = true;
      Serial.println("Server discovered at: " + serverIP.toString());
    }
  }
  udp.stop();
}

void registerSensor(String suffix, String type, float minVal, float maxVal) {
  JsonDocument doc;
  String uniqueId = baseMac + "-" + suffix;
  
  doc["id"] = uniqueId;
  doc["type"] = type;
  doc["minVal"] = minVal;
  doc["maxVal"] = maxVal;
  doc["sensor"] = true;
  
  char buffer[256];
  serializeJson(doc, buffer);
  
  mqttClient.beginMessage("register");
  mqttClient.print(buffer);
  mqttClient.endMessage();
  Serial.println("Registered: " + String(buffer));
}

void connectMQTTAndRegister() {
  Serial.print("Connecting to MQTT broker...");
  if (!mqttClient.connect(serverIP, 1883)) {
    Serial.println("Failed! Error code = " + String(mqttClient.connectError()));
    return;
  }
  Serial.println("Connected!");

  // Register each sensor uniquely
  registerSensor("TMP", "temperature", -10, 50);
  registerSensor("HUM", "humidity", 0, 100);
  registerSensor("LGT", "brightness", 0, 100);
  registerSensor("TLT", "tilt_status", 0, 1);
}

void publishReading(String suffix, String type, float value) {
  String topic = type + "/" + baseMac + "-" + suffix;
  mqttClient.beginMessage(topic);
  mqttClient.print(value);
  mqttClient.endMessage();
  Serial.println("Published to " + topic + ": " + String(value));
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Pico W Env Monitor Starting ...");

  pinMode(TILT_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  // Set ADC resolution to 12 bits (0-4095) for light sensor readings
  analogReadResolution(12);
  
  dhtSensor.begin();

  connectWiFi();
  discoverServer();
  connectMQTTAndRegister();
}

void loop() {
  if (serverFound && !mqttClient.connected()) {
    connectMQTTAndRegister();
  }
  if (serverFound && mqttClient.connected()) {
    mqttClient.poll();
  }

  unsigned long now = millis();
  if (now - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = now;

    // Read Sensors
    float t = dhtSensor.readTemperature();
    float h = dhtSensor.readHumidity();
    int rawLight = analogRead(ADC_PIN);
    float l = ((float)rawLight / 4095.0) * 100.0;
    int tilt = (digitalRead(TILT_PIN) == LOW) ? 1 : 0;
    digitalWrite(LED_PIN, tilt == 1 ? HIGH : LOW);

    // Publish valid readings
    if (!isnan(t)) publishReading("TMP", "temperature", t);
    if (!isnan(h)) publishReading("HUM", "humidity", h);
    publishReading("LGT", "brightness", l);
    publishReading("TLT", "tilt_status", tilt);
    
  }
}
