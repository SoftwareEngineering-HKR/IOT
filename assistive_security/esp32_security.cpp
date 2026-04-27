#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <string.h>

#include "config.h"
#include "esp32_devices.h"

// -----------------------------------------------------------------------------
// Network / MQTT
// -----------------------------------------------------------------------------
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
WiFiUDP udp;

IPAddress serverIP;
bool serverFound = false;

// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------
static const unsigned long UPDATE_INTERVAL_MS = 200UL;
static const unsigned long RFID_UNLOCK_TIME_MS = 10000UL;

unsigned long lastUpdateAt = 0;
unsigned long lastWifiAttemptAt = 0;
unsigned long lastMqttAttemptAt = 0;