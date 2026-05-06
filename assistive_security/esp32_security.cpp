
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

// -----------------------------------------------------------------------------
// RFID door state
// -----------------------------------------------------------------------------
bool rfidDoorOpen = false;
unsigned long rfidDoorOpenUntil = 0;

// -----------------------------------------------------------------------------
// Last values for table display
// -----------------------------------------------------------------------------
int currentRfidValue = 0;
int currentLidarMm = 0;
int currentRadarPresence = 0;
int currentRadarDistanceMm = 0;
int currentRadarSpeedCms = 0;

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------
void extractDeviceIdFromTopic(const char* topic, char* idOut, size_t idOutSize) {
  const char* slash = strchr(topic, '/');

  if (!slash) {
    snprintf(idOut, idOutSize, "%s", topic);
    return;
  }

  snprintf(idOut, idOutSize, "%s", slash + 1);
}

void buildMetricId(const char* baseId, char suffix, char* out, size_t outSize) {
  snprintf(out, outSize, "%s%c", baseId, suffix);
}