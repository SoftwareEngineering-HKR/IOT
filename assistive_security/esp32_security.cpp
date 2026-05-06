
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
static const unsigned long UPDATE_INTERVAL_MS = SENSOR_PUBLISH_INTERVAL_MS;
static const unsigned long RFID_UNLOCK_TIME_MS = 10000UL;

unsigned long lastUpdateAt = 0;
unsigned long lastWifiAttemptAt = 0;
unsigned long lastMqttAttemptAt = 0;

// -----------------------------------------------------------------------------
// Devices
// Your deployed wiring remains:
// I2C SDA = GPIO 5
// I2C SCL = GPIO 6
// LD2450 RX = GPIO 2
// LD2450 TX = GPIO 1
// -----------------------------------------------------------------------------
LidarDevice lidarDev(LIDAR_SDA_PIN);
RadarDevice radarDev(LD2450_RX_PIN, LD2450_TX_PIN);
RFIDDevice rfidDev(RC522_SDA_PIN);
