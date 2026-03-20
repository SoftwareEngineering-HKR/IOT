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