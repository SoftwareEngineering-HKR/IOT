#include "bluetooth_service.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef1"

static BluetoothService* instance = nullptr;

// ===== SERVER =====
static BLECharacteristic* characteristic;
static bool deviceConnected = false;

// ===== CLIENT =====
static BLEClient* client = nullptr;
static BLERemoteCharacteristic* remoteCharacteristic = nullptr;
static BLEAdvertisedDevice* foundDevice = nullptr;

static bool clientConnected = false;

// ===== HEARTBEAT =====
static unsigned long lastPing = 0;
static unsigned long lastPong = 0;

// ================= SERVER CALLBACKS =================
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("[BLE] Client connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Client disconnected");
    BLEDevice::startAdvertising();
  }
};

// ================= CHARACTERISTIC CALLBACKS =================
class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    std::string rx = std::string(pChar->getValue().c_str());
    String msg = String(rx.c_str());

    Serial.print("[BLE RX] ");
    Serial.println(msg);

    if (!instance) return;

    if (msg == "rdy") {
      String json =
        "{\"type\":\"register\","
        "\"deviceType\":\"light\","
        "\"maxVal\":1,\"minVal\":0,"
        "\"sensor\":false}";
      instance->sendRegister(json.c_str());
    }
    else if (msg == "reg") {
      Serial.println("[BLE] registered successfully");
    }
    else if (msg.startsWith("set;")) {
      instance->onMessage((char*)msg.c_str());
    }
    else if (msg == "pong") {
      lastPong = millis();
    }
  }
};

// ================= SCAN CALLBACK =================
class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    std::string name = advertisedDevice.getName();

    if (name.find("SE-HKR") != std::string::npos) {
      Serial.print("[BLE] Found: ");
      Serial.println(name.c_str());

      foundDevice = new BLEAdvertisedDevice(advertisedDevice);
      BLEDevice::getScan()->stop();
    }
  }
};

// ================= CLIENT CONNECT =================
bool connectToServer() {
  if (!foundDevice) return false;

  client = BLEDevice::createClient();

  Serial.println("[BLE] Connecting...");

  if (!client->connect(foundDevice)) {
    Serial.println("[BLE] Connection failed");
    return false;
  }

  BLERemoteService* service = client->getService(SERVICE_UUID);
  if (!service) {
    client->disconnect();
    return false;
  }

  remoteCharacteristic = service->getCharacteristic(CHARACTERISTIC_UUID);
  if (!remoteCharacteristic) {
    client->disconnect();
    return false;
  }

  Serial.println("[BLE] Connected to SE-HKR");
  clientConnected = true;

  delete foundDevice;
  foundDevice = nullptr;

  return true;
}

// ================= INIT =================
void BluetoothService::init() {
  instance = this;

  BLEDevice::init("ESP32_LIGHT");

  // ===== SERVER =====
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService* service = server->createService(SERVICE_UUID);

  characteristic = service->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  characteristic->setCallbacks(new CharacteristicCallbacks());
  characteristic->addDescriptor(new BLE2902());

  service->start();

  BLEDevice::getAdvertising()->start();

  // ===== SCAN =====
  BLEScan* scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  scan->setActiveScan(true);
  scan->start(0, false); // continuous scan

  Serial.println("[BLE] Init done, scanning...");
}

// ================= LOOP =================
void BluetoothService::loop() {

  // ===== AUTO CONNECT =====
  if (!clientConnected && foundDevice) {
    if (!connectToServer()) {
      Serial.println("[BLE] Retry scan...");
      BLEDevice::getScan()->start(0, false);
    }
  }

  // detect disconnect
  if (clientConnected && client && !client->isConnected()) {
    Serial.println("[BLE] Client lost → rescanning");
    clientConnected = false;
    remoteCharacteristic = nullptr;
    BLEDevice::getScan()->start(0, false);
  }

  // ===== SERVER HEARTBEAT =====
  if (!deviceConnected) return;

  unsigned long now = millis();

  if (now - lastPing > 15000) {
    String ping = "{\"type\":\"ping\"}";
    characteristic->setValue(ping.c_str());
    characteristic->notify();
    lastPing = now;
  }

  if (now - lastPong > 30000) {
    BLEDevice::deinit(true);
    delay(500);
    ESP.restart();
  }
}

// ================= API =================
bool BluetoothService::connected() {
  return deviceConnected;
}

void BluetoothService::sendRegister(const char* json) {
  if (!characteristic) return;
  characteristic->setValue(json);
  characteristic->notify();
}

void BluetoothService::sendPing() {
  if (!characteristic) return;
  const char* ping = "{\"type\":\"ping\"}";
  characteristic->setValue(ping);
  characteristic->notify();
}

void BluetoothService::sendValue(const char* topic, const char* msg) {
  if (!characteristic) return;
  characteristic->setValue(msg);
  characteristic->notify();
}

void BluetoothService::sendToRemote(const char* msg) {
  if (remoteCharacteristic && clientConnected) {
    remoteCharacteristic->writeValue((uint8_t*)msg, strlen(msg));
  }
}
