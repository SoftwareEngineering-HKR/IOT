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

void buildTopic(const char* type, const char* id, char* out, size_t outSize) {
  snprintf(out, outSize, "%s/%s", type, id);
}

const char* radarMotionText(int speedCms, int presence) {
  if (presence == 0) return "none";
  if (speedCms < 0) return "approach";
  if (speedCms > 0) return "away";
  return "still";
}

int findNearestValidTargetIndex(RadarDevice* radar) {
  int bestIndex = -1;
  uint16_t bestDistance = 0;

  for (int i = 0; i < 3; i++) {
    const RadarDevice::TargetInfo& target = radar->getTarget(i);

    if (!target.valid) {
      continue;
    }

    if (bestIndex == -1 || target.distance_mm < bestDistance) {
      bestIndex = i;
      bestDistance = target.distance_mm;
    }
  }

  return bestIndex;
}

// -----------------------------------------------------------------------------
// Table display on ESP32 Serial Monitor
// -----------------------------------------------------------------------------
void printTableHeaderOnce() {
  static bool printed = false;

  if (printed) {
    return;
  }

  Serial.println();
  Serial.println("+------+----------+----------------+-------------------+-----------------+--------------+");
  Serial.println("| RFID | LIDAR_mm | RADAR_presence | RADAR_distance_mm | RADAR_speed_cms | RADAR_motion |");
  Serial.println("+------+----------+----------------+-------------------+-----------------+--------------+");

  printed = true;
}

void printSensorTableRow() {
  printTableHeaderOnce();

  Serial.printf(
    "| %-4d | %-8d | %-14d | %-17d | %-15d | %-12s |\n",
    currentRfidValue,
    currentLidarMm,
    currentRadarPresence,
    currentRadarDistanceMm,
    currentRadarSpeedCms,
    radarMotionText(currentRadarSpeedCms, currentRadarPresence)
  );
}

// -----------------------------------------------------------------------------
// Backend registration
// -----------------------------------------------------------------------------
void publishRegisterPayload(const char* id, const char* type, int maxVal, int minVal, bool sensor) {
  
  
  JsonDocument doc;
  doc["id"] = id;
  doc["type"] = type;
  doc["maxVal"] = maxVal;
  doc["minVal"] = minVal;
  doc["sensor"] = sensor;

  char buffer[160];
  serializeJson(doc, buffer, sizeof(buffer));

  mqttClient.beginMessage("register");
  mqttClient.print(buffer);
  bool ok = mqttClient.endMessage();

  Serial.print("[register] ");
  Serial.print(buffer);
  Serial.println(ok ? " OK" : " FAILED");
}

void registerDeviceFromTopic(Device* device, const char* backendType, int maxVal, int minVal) {
  char topic[80];
  char id[64];

  device->getTopic(topic, sizeof(topic));
  extractDeviceIdFromTopic(topic, id, sizeof(id));

  publishRegisterPayload(id, backendType, maxVal, minVal, true);
}

void registerRadarExtraMetrics() {
  char radarBaseTopic[80];
  char radarBaseId[64];

  radarDev.getTopic(radarBaseTopic, sizeof(radarBaseTopic));
  extractDeviceIdFromTopic(radarBaseTopic, radarBaseId, sizeof(radarBaseId));

  char distanceId[72];
  char speedId[72];

  buildMetricId(radarBaseId, 'D', distanceId, sizeof(distanceId));
  buildMetricId(radarBaseId, 'S', speedId, sizeof(speedId));

  // Radar distance in mm.
  publishRegisterPayload(distanceId, "photo", 6000, 0, true);

  // Radar speed in cm/s.
  publishRegisterPayload(speedId, "motion", 1000, -1000, true);

  // Do NOT register radar status as "button".
  // RFID is the only button device.
}

void registerAllDevicesForBackend() {
  // LIDAR distance.
  registerDeviceFromTopic(&lidarDev, "photo", LIDAR_MAX_VALUE, LIDAR_MIN_VALUE);

  // RADAR presence.
  registerDeviceFromTopic(&radarDev, "motion", 1, 0);

  // RADAR distance and speed.
  registerRadarExtraMetrics();

  // RFID door button.
  registerDeviceFromTopic(&rfidDev, "button", 1, 0);
}

// -----------------------------------------------------------------------------
// MQTT publish
// -----------------------------------------------------------------------------
bool publishIntTopic(const char* topic, int value) {
  if (!mqttClient.connected()) {
    return false;
  }

  mqttClient.beginMessage(topic);
  mqttClient.print(value);
  bool ok = mqttClient.endMessage();

  return ok;
}

// -----------------------------------------------------------------------------
// Read sensors and publish every 200 ms
// -----------------------------------------------------------------------------
void readPublishAndDisplayAllSensors200ms() {
  // ---------------------------------------------------------------------------
  // LIDAR
  // ---------------------------------------------------------------------------
  int lidarReading = lidarDev.getReading();

  if (lidarReading < 0) {
    lidarReading = 0;
  }

  currentLidarMm = lidarReading;

  char lidarTopic[80];
  lidarDev.getTopic(lidarTopic, sizeof(lidarTopic));

  // ---------------------------------------------------------------------------
  // RADAR
  // ---------------------------------------------------------------------------
  radarDev.getReading();

  char radarBaseTopic[80];
  char radarBaseId[64];

  radarDev.getTopic(radarBaseTopic, sizeof(radarBaseTopic));
  extractDeviceIdFromTopic(radarBaseTopic, radarBaseId, sizeof(radarBaseId));

  currentRadarPresence = radarDev.getTargetCount() > 0 ? 1 : 0;
  currentRadarDistanceMm = 0;
  currentRadarSpeedCms = 0;

  int bestTargetIndex = findNearestValidTargetIndex(&radarDev);

  if (bestTargetIndex >= 0) {
    const RadarDevice::TargetInfo& target = radarDev.getTarget(bestTargetIndex);

    currentRadarDistanceMm = target.distance_mm;
    currentRadarSpeedCms = target.speed_cms;
  }

  char radarDistanceId[72];
  char radarSpeedId[72];

  char radarDistanceTopic[80];
  char radarSpeedTopic[80];

  buildMetricId(radarBaseId, 'D', radarDistanceId, sizeof(radarDistanceId));
  buildMetricId(radarBaseId, 'S', radarSpeedId, sizeof(radarSpeedId));

  buildTopic("photo", radarDistanceId, radarDistanceTopic, sizeof(radarDistanceTopic));
  buildTopic("motion", radarSpeedId, radarSpeedTopic, sizeof(radarSpeedTopic));

  // ---------------------------------------------------------------------------
  // RFID
  // ---------------------------------------------------------------------------
  unsigned long now = millis();

  int rfidTouched = rfidDev.getReading();

  if (rfidTouched == 1) {
    rfidDoorOpen = true;
    rfidDoorOpenUntil = now + RFID_UNLOCK_TIME_MS;
  }

  if (rfidDoorOpen && (long)(now - rfidDoorOpenUntil) >= 0) {
    rfidDoorOpen = false;
  }

  currentRfidValue = rfidDoorOpen ? 1 : 0;

  char rfidTopic[80];
  rfidDev.getTopic(rfidTopic, sizeof(rfidTopic));

  // ---------------------------------------------------------------------------
  // Publish all values every 200 ms
  // ---------------------------------------------------------------------------
  publishIntTopic(rfidTopic, currentRfidValue);
  publishIntTopic(lidarTopic, currentLidarMm);
  publishIntTopic(radarBaseTopic, currentRadarPresence);
  publishIntTopic(radarDistanceTopic, currentRadarDistanceMm);
  publishIntTopic(radarSpeedTopic, currentRadarSpeedCms);

  // ---------------------------------------------------------------------------
  // Local serial table display
  // ---------------------------------------------------------------------------
  printSensorTableRow();
}

// -----------------------------------------------------------------------------
// WiFi / discovery / MQTT
// -----------------------------------------------------------------------------
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("[wifi] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("[wifi] Connected IP: ");
  Serial.println(WiFi.localIP());
}

void discoverServer() {
#if USE_STATIC_SERVER_IP
  serverIP = IPAddress(
    STATIC_SERVER_IP_1,
    STATIC_SERVER_IP_2,
    STATIC_SERVER_IP_3,
    STATIC_SERVER_IP_4
  );

  serverFound = true;

  Serial.print("[server] Using static backend IP: ");
  Serial.println(serverIP);
#else
  Serial.println("[server] Discovering backend with UDP broadcast...");
  udp.begin(UDP_DISCOVERY_PORT);

  while (!serverFound) {
    Serial.println("[server] Sending discovery packet...");

    udp.beginPacket(IPAddress(255, 255, 255, 255), UDP_DISCOVERY_PORT);
    udp.print(UDP_DISCOVERY_MESSAGE);
    udp.endPacket();

    unsigned long start = millis();

    while (millis() - start < DISCOVERY_RETRY_INTERVAL_MS) {
      int packetSize = udp.parsePacket();

      if (packetSize > 0) {
        char response[64];
        int n = udp.read(response, sizeof(response) - 1);

        if (n > 0) {
          response[n] = '\0';

          Serial.print("[server] UDP response: ");
          Serial.println(response);

          if (strcmp(response, UDP_DISCOVERY_RESPONSE) == 0) {
            serverIP = udp.remoteIP();
            serverFound = true;

            Serial.print("[server] Discovered backend IP: ");
            Serial.println(serverIP);
            break;
          }
        }
      }

      delay(10);
    }
  }

  udp.stop();
#endif
}

bool connectMqttAndRegister() {
  if (!serverFound) {
    Serial.println("[mqtt] Cannot connect: server IP not found");
    return false;
  }

  if (mqttClient.connected()) {
    return true;
  }

  uint8_t mac[6];
  WiFi.macAddress(mac);

  char clientId[40];
  snprintf(
    clientId,
    sizeof(clientId),
    "esp32s3-%02X%02X%02X%02X%02X%02X",
    mac[0],
    mac[1],
    mac[2],
    mac[3],
    mac[4],
    mac[5]
  );

  mqttClient.setId(clientId);

  Serial.print("[mqtt] Connecting to ");
  Serial.print(serverIP);
  Serial.print(":");
  Serial.println(MQTT_BROKER_PORT);

  if (!mqttClient.connect(serverIP, MQTT_BROKER_PORT)) {
    Serial.print("[mqtt] Connect failed, error=");
    Serial.println(mqttClient.connectError());
    return false;
  }

  Serial.println("[mqtt] Connected");
  Serial.println("[mqtt] Registering backend devices...");

  registerAllDevicesForBackend();

  Serial.println("[mqtt] Registration sent");

  rfidDoorOpen = false;
  rfidDoorOpenUntil = 0;
  currentRfidValue = 0;

  return true;
}

// -----------------------------------------------------------------------------
// Arduino setup / loop
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-S3 Security Monitor Starting...");

  WiFi.mode(WIFI_STA);

  // Keep your deployed I2C pins exactly:
  // SDA = GPIO 5
  // SCL = GPIO 6
  Wire.begin(LIDAR_SDA_PIN, LIDAR_SCL_PIN);
  Wire.setClock(400000);

  lidarDev.init();
  radarDev.init();
  rfidDev.init();

  connectWiFi();
  discoverServer();
  connectMqttAndRegister();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();

    if (now - lastWifiAttemptAt >= WIFI_RECONNECT_INTERVAL_MS) {
      lastWifiAttemptAt = now;
      connectWiFi();
    }

    return;
  }

  if (!mqttClient.connected()) {
    unsigned long now = millis();

    if (now - lastMqttAttemptAt >= MQTT_RECONNECT_INTERVAL_MS) {
      lastMqttAttemptAt = now;
      connectMqttAndRegister();
    }
  }

  if (mqttClient.connected()) {
    mqttClient.poll();
  }

  unsigned long now = millis();

  if (mqttClient.connected() && now - lastUpdateAt >= UPDATE_INTERVAL_MS) {
    lastUpdateAt = now;
    readPublishAndDisplayAllSensors200ms();
  }
}