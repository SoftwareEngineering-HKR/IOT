#include "bluetooth_service.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef1"

static BluetoothService* instance = nullptr;

static BLECharacteristic* characteristic;
static bool deviceConnected = false;

static unsigned long lastPing = 0;
static unsigned long lastPong = 0;

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

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    std::string rx = std::string(pChar->getValue().c_str());
    String msg = String(rx.c_str());

    Serial.print("[BLE RX] ");
    Serial.println(msg);

    if (!instance) return;


    if (msg == "rdy") {
      Serial.println("[BLE] rdy received → sending register");

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
      Serial.println("[BLE] set command received");

      // Forward raw command to user system if needed
      instance->onMessage((char*)msg.c_str());
    }

    else if (msg == "pong") {
      Serial.println("[BLE] pong received");
      lastPong = millis();
    }
  }
};

void BluetoothService::init() {
  instance = this;

  BLEDevice::init("ESP32_LIGHT");

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

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->start();

  Serial.println("[BLE] Service started, advertising...");
}

void BluetoothService::loop() {
  if (!deviceConnected) return;

  unsigned long now = millis();

  if (now - lastPing > 15000) {
    String ping = "{\"type\":\"ping\"}";

    characteristic->setValue(ping.c_str());
    characteristic->notify();

    Serial.println("[BLE] ping sent");

    lastPing = now;
  }

  if (now - lastPong > 30000) {
    Serial.println("[BLE] pong timeout → restarting BLE");

    BLEDevice::deinit(true);
    delay(500);
    ESP.restart();
  }
}

bool BluetoothService::connected() {
  return deviceConnected;
}


void BluetoothService::sendRegister(const char* json) {
  if (!characteristic) return;

  characteristic->setValue(json);
  characteristic->notify();

  Serial.println("[BLE] register sent");
}

void BluetoothService::sendPing() {
  if (!characteristic) return;

  const char* ping = "{\"type\":\"ping\"}";
  characteristic->setValue(ping);
  characteristic->notify();
}

void BluetoothService::sendValue(const char* topic, const char* msg) {
  if (!characteristic) return;

  String payload = String(msg);

  characteristic->setValue(payload.c_str());
  characteristic->notify();

  Serial.println("[BLE] value sent");
}
