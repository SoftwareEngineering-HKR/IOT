#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <ArduinoMqttClient.h>
#include <HTTPClient.h>
#include <DHT.h>
#include "config.h" 
#include "pico_devices.h" 


#define DHT_TYPE DHT11
DHT dhtSensor(DHT_PIN, DHT_TYPE);

// MQTT
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
WiFiUDP udp;

// Server discovery
const int UDP_PORT = 1883; 
IPAddress serverIP;
bool serverFound = false;

// Update interval in milliseconds
const unsigned long UPDATE_INTERVAL = 60000; // 60s
unsigned long lastUpdate = 0;

// Device instances
DHTTempDevice tempDev(DHT_PIN);
DHTHumDevice  humDev(DHT_PIN);
LightDevice   lightDev(ADC_PIN);
TiltDevice    tiltDev(TILT_PIN);

// Array of device pointers for easy iteration
Device* myDevices[] = {&tempDev, &humDev, &lightDev, &tiltDev};
const int deviceCount = sizeof(myDevices) / sizeof(myDevices[0]);

void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
}

void discoverServer() {
  Serial.println("Broadcasting UDP to discover server...");
  udp.begin(UDP_PORT);
  
  while (!serverFound) {
    udp.beginPacket(IPAddress(255, 255, 255, 255), UDP_PORT);
    udp.print("DISCOVER_SERVER"); 
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
void connectMQTTAndRegister() {
  Serial.print("Connecting to MQTT broker...");
  if (!mqttClient.connect(serverIP, 1883)) {
    Serial.println("Failed! Error code = " + String(mqttClient.connectError()));
    return;
  }
  Serial.println("Connected!");

  // Use the `getRegistration` method from the Device class
  char buffer[128];
  for (int i = 0; i < deviceCount; i++) {
    myDevices[i]->getRegistration(buffer);
    
    mqttClient.beginMessage("registration");
    mqttClient.print(buffer);
    mqttClient.endMessage();
  }
}
void checkAndAlertDiscord(int t, int h, int l, int tilt) {
  if (WiFi.status() != WL_CONNECTED) return;

  bool tempAlert  = (t != -1 && t > TEMP_THRESHOLD);
  bool humAlert   = (h != -1 && h > HUMIDITY_THRESHOLD);
  bool lightAlert = (l != -1 && l < LIGHT_THRESHOLD);
  bool tiltAlert  = (tilt == 1);

  if (!tempAlert && !humAlert && !lightAlert && !tiltAlert) return;

  String msg = "EMERGENCY ALERT! Temp: " + (t == -1 ? String("N/A") : String(t) + "C") + 
               ", Hum: " + (h == -1 ? String("N/A") : String(h) + "%") + 
               ", Light: " + String(l) + "%, Tilt: " + (tilt ? "Tilted" : "Safe");

  String payload = "{\"content\": \"" + msg + "\"}";

  WiFiClientSecure client;
  client.setInsecure(); 
  HTTPClient http;
  
  http.begin(client, DISCORD_WEBHOOK_URL);
  http.addHeader("Content-Type", "application/json");
  
  int code = http.POST(payload);
  if (code > 0 && code / 100 == 2) Serial.println("Emergency alert sent to Discord!");
  http.end();
}
void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(TILT_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  analogReadResolution(16);
  
  dhtSensor.begin();

  // Initialize all devices
  for (int i = 0; i < deviceCount; i++) {
    myDevices[i]->init();
  }

  connectWiFi();
  discoverServer();
  connectMQTTAndRegister();
}