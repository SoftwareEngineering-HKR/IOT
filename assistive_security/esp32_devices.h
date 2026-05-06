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

  