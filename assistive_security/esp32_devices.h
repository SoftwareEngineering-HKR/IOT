#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <MFRC522_I2C.h>
#include <math.h>
#include <string.h>

#include "config.h"

// -----------------------------------------------------------------------------
// ESP32-only device base class
// -----------------------------------------------------------------------------
// This replaces only the small part of device.h/device.cpp needed by this
// firmware. Do not include ../include/device.h here and do not modify device.cpp.
// Needed by this code:
//   - type / pin / maxVal / mac storage
//   - constructor
//   - getTopic()
//   - virtual init()
//   - virtual getReading()
// -----------------------------------------------------------------------------
class SecurityDevice {
protected:
  uint8_t mac[6] = {0, 0, 0, 0, 0, 0};
  char type[10];
  int maxVal;
  int pin;

public:
  SecurityDevice(const char* deviceType, int devicePin, int deviceMaxVal)
    : maxVal(deviceMaxVal), pin(devicePin) {
    strncpy(type, deviceType, sizeof(type));
    type[sizeof(type) - 1] = '\0';
  }

  virtual ~SecurityDevice() = default;

  void getTopic(char* buffer, size_t size) const {
    char macBuffer[13];
    macToString(mac, macBuffer);
    snprintf(buffer, size, "%s/%s", type, macBuffer);
  }

  virtual void handleMessage(const char* msg) {
    (void)msg;
  }

  virtual int getReading() {
    return -1;
  }

  virtual void init() = 0;


  

private:
  static void macToString(const uint8_t inMac[6], char buffer[13]) {
    static const char hex[] = "0123456789ABCDEF";
    int j = 0;

    for (int i = 0; i < 6; i++) {
      buffer[j++] = hex[(inMac[i] >> 4) & 0x0F];
      buffer[j++] = hex[inMac[i] & 0x0F];
    }

    buffer[j] = '\0';
  }
};

// -----------------------------------------------------------------------------
// Shared hardware objects
// -----------------------------------------------------------------------------
#define C3_LED_PIN BOARD_LED_PIN

VL53L0X lidar;
MFRC522 rfid(RC522_I2C_ADDRESS, RC522_RST_PIN);

inline void assignDerivedMac(uint8_t outMac[6], uint8_t offset) {
  WiFi.macAddress(outMac);
  outMac[5] = (uint8_t)(outMac[5] + offset);
  if (outMac[5] == 0x00) {
    outMac[5] = offset;
  }
}

// -----------------------------------------------------------------------------
// LIDAR VL53L0X
// -----------------------------------------------------------------------------
class LidarDevice : public SecurityDevice {
public:
  explicit LidarDevice(int lidarPin)
    : SecurityDevice("photo", lidarPin, LIDAR_MAX_VALUE) {}

  void init() override {
    assignDerivedMac(this->mac, 1);

    lidar.setTimeout(500);

    if (!lidar.init()) {
      Serial.println("[lidar] Failed to detect and initialize VL53L0X");
      return;
    }

    lidar.startContinuous();
    Serial.println("[lidar] VL53L0X ready");
  }

  