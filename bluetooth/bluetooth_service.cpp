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

static bool deviceConnected = false;

static unsigned long lastPing = 0;
static unsigned long lastPong = 0;

// ================= SERVER CALLBACKS =================

class ServerCallbacks : public BLEServerCallbacks {

    void onConnect(BLEServer* pServer) override {

        deviceConnected = true;

        lastPong = millis();

        Serial.println("[BLE] Client connected");
    }

    void onDisconnect(BLEServer* pServer) override {

        deviceConnected = false;

        Serial.println("[BLE] Client disconnected");

        delay(200);

        pServer->startAdvertising();

        Serial.println("[BLE] Advertising restarted");
    }
};

// ================= CHARACTERISTIC CALLBACKS =================

class CharacteristicCallbacks : public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic* pChar) override {

        std::string rx = pChar->getValue();

        String msg = String(rx.c_str());

        Serial.print("[BLE RX] ");
        Serial.println(msg);

        // ===== READY =====

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

        // ===== REGISTER CONFIRM =====

        else if (msg == "reg") {

            Serial.println("[BLE] Device registered");
        }

        // ===== SET LED =====

        else if (msg.startsWith("set;")) {

            int value = msg.substring(4).toInt();

            digitalWrite(BLE_PIN, value ? HIGH : LOW);

            Serial.print("[BLE] LED set to ");
            Serial.println(value);
        }

        // ===== PING FROM SERVER =====

        else if (
            msg == "ping" ||
            msg.indexOf("\"type\":\"ping\"") != -1
        ) {

            characteristic->setValue("pong");
            characteristic->notify();

            Serial.println("[BLE] Pong sent");
        }

        // ===== PONG FROM SERVER =====

        else if (msg == "pong") {

            lastPong = millis();

            Serial.println("[BLE] Pong received");
        }
    }
};

// ================= INIT =================

void BluetoothService::init() {

    pinMode(BLE_PIN, OUTPUT);

    digitalWrite(BLE_PIN, LOW);

    BLEDevice::init("SE-HKR-light");

    BLEServer* server =
        BLEDevice::createServer();

    server->setCallbacks(
        new ServerCallbacks()
    );

    BLEService* service =
        server->createService(SERVICE_UUID);

    characteristic =
        service->createCharacteristic(
            CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY
        );

    characteristic->addDescriptor(
        new BLE2902()
    );

    characteristic->setCallbacks(
        new CharacteristicCallbacks()
    );

    service->start();

    BLEAdvertising* advertising =
        BLEDevice::getAdvertising();

    advertising->addServiceUUID(SERVICE_UUID);

    advertising->setScanResponse(true);

    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);

    BLEDevice::startAdvertising();

    Serial.println("[BLE] Advertising...");
    Serial.println("[BLE] Ready");
}

// ================= LOOP =================

void BluetoothService::loop() {

    // send ping every 15s

    if (
        deviceConnected &&
        millis() - lastPing > 15000
    ) {

        lastPing = millis();

        characteristic->setValue(
            "{\"type\":\"ping\"}"
        );

        characteristic->notify();

        Serial.println("[BLE] Ping sent");
    }

    // reconnect if no pong in 30s

    if (
        deviceConnected &&
        millis() - lastPong > 30000
    ) {

        Serial.println(
            "[BLE] Pong timeout, restarting advertising"
        );

        deviceConnected = false;

        BLEDevice::startAdvertising();
    }
}
