#include "bluetooth_service.h"
#include "config.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef1"
#define BLE_PIN 2

static BLECharacteristic* characteristic;

class ServerCallbacks : public BLEServerCallbacks {

    void onConnect(BLEServer* pServer) override {
        Serial.println("[BLE] Client connected");
    }

    void onDisconnect(BLEServer* pServer) override {
        Serial.println("[BLE] Client disconnected");

        BLEDevice::startAdvertising();
    }
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {

void onWrite(BLECharacteristic* pChar) override {

    std::string rx = pChar->getValue();

    String msg = String(rx.c_str());

    Serial.print("[BLE RX] ");
    Serial.println(msg);

    if (msg == "rdy") {

        String json =
            "{\"type\":\"register\","
            "\"deviceType\":\"light\","
            "\"maxVal\":1,"
            "\"minVal\":0,"
            "\"sensor\":false}";

        characteristic->setValue(json.c_str());
        characteristic->notify();

        Serial.println("[BLE] Register sent");
    }

    else if (msg.startsWith("set;")) {

        int value = msg.substring(4).toInt();

        digitalWrite(BLE_PIN, value ? HIGH : LOW);

        Serial.print("[BLE] LED set to ");
        Serial.println(value);
    }

    else if (msg.indexOf("\"type\":\"ping\"") != -1) {

        characteristic->setValue("pong");
        characteristic->notify();

        Serial.println("[BLE] Pong sent");
    }
}
};

void BluetoothService::init() {

    pinMode(BLE_PIN, OUTPUT);

    BLEDevice::init("SE-HKR-light");

    BLEServer* server = BLEDevice::createServer();

    server->setCallbacks(new ServerCallbacks());

    BLEService* service =
        server->createService(SERVICE_UUID);

    characteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    characteristic->addDescriptor(new BLE2902());

    characteristic->setCallbacks(
        new CharacteristicCallbacks()
    );

    service->start();

    BLEAdvertising* advertising =
        BLEDevice::getAdvertising();

    advertising->start();

    Serial.println("[BLE] Advertising...");
}

void BluetoothService::loop() {
}
