#pragma once

#include <WiFi.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <MFRC522_I2C.h>
#include <math.h>
#include "config.h"
#include "../include/device.h"

// Your deployed wiring stays the same:
// I2C SDA = GPIO 5, I2C SCL = GPIO 6
// LIDAR and RC522_I2C share the same I2C bus.

#define C3_LED_PIN BOARD_LED_PIN

VL53L0X lidar;
MFRC522 rfid(RC522_I2C_ADDRESS, RC522_RST_PIN);   // RC522_RST_PIN may remain -1, like your working code.

inline void assignDerivedMac(uint8_t mac[6], uint8_t offset) {
  WiFi.macAddress(mac);
  mac[5] = (uint8_t)(mac[5] + offset);
  if (mac[5] == 0x00) mac[5] = offset;
}

class LidarDevice : public Device {
public:
  LidarDevice(int pin) : Device("photo", pin, LIDAR_MAX_VALUE) {}

  void init() override {
    assignDerivedMac(this->mac, 1);

    lidar.setTimeout(500);
    if (!lidar.init()) {
      Serial.println("[lidar] Failed to detect and initialize VL53L0X");
    } else {
      lidar.startContinuous();
      Serial.println("[lidar] VL53L0X ready");
    }
  }

  int getReading() override {
    int distance = lidar.readRangeContinuousMillimeters();
    if (lidar.timeoutOccurred() || distance < LIDAR_MIN_VALUE || distance > LIDAR_MAX_VALUE) {
      return -1;
    }
    return distance;
  }
};

class RadarDevice : public Device {
public:
  struct TargetInfo {
    bool valid = false;
    int16_t x_mm = 0;
    int16_t y_mm = 0;
    int16_t speed_cms = 0;
    uint16_t resolution_mm = 0;
    uint16_t distance_mm = 0;
  };

private:
  static const int FRAME_LEN = 30;
  static const int TARGETS = 3;

  int rxPin;
  int txPin;
  uint8_t frame[FRAME_LEN];
  TargetInfo targets[TARGETS];
  int targetCount = 0;
  uint16_t nearestDistance = 0;

  void clearTargets() {
    targetCount = 0;
    nearestDistance = 0;
    for (int i = 0; i < TARGETS; i++) targets[i] = TargetInfo{};
  }

  static int16_t decodeSignedMag16(uint8_t lowByte, uint8_t highByte) {
    int16_t value = ((highByte & 0x7F) << 8) | lowByte;
    if ((highByte & 0x80) == 0) value = -value;
    return value;
  }

  bool readFrame() {
    while (Serial1.available() >= FRAME_LEN) {
      if (Serial1.peek() != 0xAA) {
        Serial1.read();
        continue;
      }

      size_t n = Serial1.readBytes(frame, FRAME_LEN);
      if (n != FRAME_LEN) return false;

      if (frame[0] == 0xAA && frame[1] == 0xFF && frame[2] == 0x03 && frame[3] == 0x00 &&
          frame[28] == 0x55 && frame[29] == 0xCC) {
        return true;
      }
    }
    return false;
  }

  void parseFrame() {
    clearTargets();

    for (int i = 0; i < TARGETS; i++) {
      int base = 4 + i * 8;

      int16_t x = decodeSignedMag16(frame[base + 0], frame[base + 1]);
      int16_t y = decodeSignedMag16(frame[base + 2], frame[base + 3]);
      int16_t speed = decodeSignedMag16(frame[base + 4], frame[base + 5]);
      uint16_t resolution = (frame[base + 7] << 8) | frame[base + 6];
      uint16_t distance = (uint16_t)sqrtf((float)x * (float)x + (float)y * (float)y);

      targets[i].x_mm = x;
      targets[i].y_mm = y;
      targets[i].speed_cms = speed;
      targets[i].resolution_mm = resolution;
      targets[i].distance_mm = distance;
      targets[i].valid = distance > 0;

      if (targets[i].valid) {
        targetCount++;
        if (nearestDistance == 0 || distance < nearestDistance) nearestDistance = distance;
      }
    }
  }

public:
  RadarDevice(int rxPin, int txPin)
      : Device("motion", rxPin, RADAR_MAX_VALUE), rxPin(rxPin), txPin(txPin) {}

  void init() override {
    assignDerivedMac(this->mac, 2);
    Serial.printf("[radar] LD2450 init on RX=%d TX=%d\n", rxPin, txPin);
    Serial1.begin(LD2450_BAUD_RATE, SERIAL_8N1, rxPin, txPin);
    Serial1.setRxBufferSize(1024);
    delay(200);
  }

  int getReading() override {
    if (!readFrame()) {
      clearTargets();
      return 0;
    }
    parseFrame();
    return targetCount;
  }

  int getTargetCount() const { return targetCount; }
  uint16_t getNearestDistance() const { return nearestDistance; }

  const TargetInfo& getTarget(int index) const {
    static TargetInfo emptyTarget{};
    if (index < 0 || index >= TARGETS) return emptyTarget;
    return targets[index];
  }
};

class RFIDDevice : public Device {
public:
  RFIDDevice(int pin) : Device("button", pin, RFID_MAX_VALUE) {}

  void init() override {
    assignDerivedMac(this->mac, 3);

    pinMode(C3_LED_PIN, OUTPUT);
    digitalWrite(C3_LED_PIN, HIGH);

    Serial.println("[rfid] Initializing RC522 I2C on shared SDA=5 SCL=6...");
    rfid.PCD_Init();
    delay(50);
    Serial.println("[rfid] RC522 I2C init done");
  }

  int getReading() override {
    if (!rfid.PICC_IsNewCardPresent()) return 0;

    if (!rfid.PICC_ReadCardSerial()) {
      Serial.println("[rfid] Card present but UID read failed");
      return 0;
    }

    Serial.print("[rfid] Key fob/card detected UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
      Serial.print(rfid.uid.uidByte[i], HEX);
      if (i < rfid.uid.size - 1) Serial.print(":");
    }
    Serial.println();

    digitalWrite(C3_LED_PIN, LOW);
    
    digitalWrite(C3_LED_PIN, HIGH);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return 1;
  }
};
