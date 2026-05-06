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

  

  int getReading() override {
    int distance = lidar.readRangeContinuousMillimeters();

    if (lidar.timeoutOccurred() || distance < LIDAR_MIN_VALUE || distance > LIDAR_MAX_VALUE) {
      return -1;
    }

    return distance;
  }
};

// -----------------------------------------------------------------------------
// LD2450 Radar
// -----------------------------------------------------------------------------
// IMPORTANT: this keeps your original pin order. No auto swap.
// The firmware calls:
//   Serial1.begin(LD2450_BAUD_RATE, SERIAL_8N1, LD2450_RX_PIN, LD2450_TX_PIN)
// through the values passed from esp32_security.cpp.
#ifndef RADAR_FRAME_STALE_TIMEOUT_MS
#define RADAR_FRAME_STALE_TIMEOUT_MS 750UL
#endif

#ifndef RADAR_DEBUG_PRINT_MS
#define RADAR_DEBUG_PRINT_MS 2000UL
#endif

class RadarDevice : public SecurityDevice {
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
  static const int RAW_SAMPLE_LEN = 64;

  int ld2450RxPin;
  int ld2450TxPin;

  uint8_t frame[FRAME_LEN];
  size_t frameIndex = 0;

  TargetInfo targets[TARGETS];
  int targetCount = 0;
  uint16_t nearestDistance = 0;

  unsigned long lastFrameAt = 0;
  unsigned long lastDebugPrintAt = 0;
  bool hasReceivedFrame = false;

  

  uint32_t bytesSeen = 0;
  uint32_t framesSeen = 0;
  uint32_t badFramesSeen = 0;

  uint8_t rawSample[RAW_SAMPLE_LEN];
  int rawSampleCount = 0;
  bool printedRawSample = false;

  void clearTargets() {
    targetCount = 0;
    nearestDistance = 0;

    for (int i = 0; i < TARGETS; i++) {
      targets[i] = TargetInfo{};
    }
  }

  static int16_t decodeSignedMag16(uint8_t lowByte, uint8_t highByte) {
    int16_t value = ((highByte & 0x7F) << 8) | lowByte;
    if ((highByte & 0x80) == 0) {
      value = -value;
    }
    return value;
  }

  void resetFrameParser() {
    frameIndex = 0;
  }

  void restartFrameParserWithByte(uint8_t b) {
    if (b == 0xAA) {
      frame[0] = b;
      frameIndex = 1;
    } else {
      frameIndex = 0;
    }
  }

  bool pushRadarByte(uint8_t b) {
    bytesSeen++;

    if (rawSampleCount < RAW_SAMPLE_LEN) {
      rawSample[rawSampleCount++] = b;
    }

    if (frameIndex == 0) {
      if (b == 0xAA) {
        frame[0] = b;
        frameIndex = 1;
      }
      return false;
    }

    if (frameIndex == 1) {
      if (b == 0xFF) {
        frame[1] = b;
        frameIndex = 2;
      } else {
        restartFrameParserWithByte(b);
      }
      return false;
    }


    if (frameIndex == 2) {
      if (b == 0x03) {
        frame[2] = b;
        frameIndex = 3;
      } else {
        restartFrameParserWithByte(b);
      }
      return false;
    }

    if (frameIndex == 3) {
      if (b == 0x00) {
        frame[3] = b;
        frameIndex = 4;
      } else {
        restartFrameParserWithByte(b);
      }
      return false;
    }

    frame[frameIndex++] = b;

    if (frameIndex < FRAME_LEN) {
      return false;
    }

    bool validFrame = frame[28] == 0x55 && frame[29] == 0xCC;
    resetFrameParser();

    if (!validFrame) {
      badFramesSeen++;
      restartFrameParserWithByte(b);
      return false;
    }

    framesSeen++;
    return true;
  }

  void parseFrame() {
    clearTargets();

    for (int i = 0; i < TARGETS; i++) {
      int base = 4 + i * 8;

      int16_t x = decodeSignedMag16(frame[base + 0], frame[base + 1]);
      int16_t y = decodeSignedMag16(frame[base + 2], frame[base + 3]);
      int16_t speed = decodeSignedMag16(frame[base + 4], frame[base + 5]);
      uint16_t resolution = ((uint16_t)frame[base + 7] << 8) | frame[base + 6];
      uint16_t distance = (uint16_t)sqrtf((float)x * (float)x + (float)y * (float)y);

      targets[i].x_mm = x;
      targets[i].y_mm = y;
      targets[i].speed_cms = speed;
      targets[i].resolution_mm = resolution;
      targets[i].distance_mm = distance;
      targets[i].valid = distance > 0;

      if (targets[i].valid) {
        targetCount++;
        if (nearestDistance == 0 || distance < nearestDistance) {
          nearestDistance = distance;
        }
      }
    }
  }


  

  bool processRadarSerial() {
    bool updated = false;

    while (Serial1.available() > 0) {
      uint8_t b = (uint8_t)Serial1.read();

      if (pushRadarByte(b)) {
        parseFrame();
        lastFrameAt = millis();
        hasReceivedFrame = true;
        updated = true;
      }
    }

    return updated;
  }

  void flushRadarSerial() {
    while (Serial1.available() > 0) {
      Serial1.read();
    }
  }

  void printRawSampleIfAvailable() {
    if (printedRawSample || rawSampleCount <= 0) {
      return;
    }

    printedRawSample = true;
    Serial.print("[radar] first raw bytes:");
    for (int i = 0; i < rawSampleCount; i++) {
      Serial.printf(" %02X", rawSample[i]);
    }
    Serial.println();
  }

  void printRadarDebugIfNeeded() {
    unsigned long now = millis();

    if ((unsigned long)(now - lastDebugPrintAt) < RADAR_DEBUG_PRINT_MS) {
      return;
    }

    lastDebugPrintAt = now;

    Serial.printf(
      "[radar] status bytes=%lu frames=%lu bad=%lu targets=%d LD2450_RX_PIN=%d LD2450_TX_PIN=%d\n",
      (unsigned long)bytesSeen,
      (unsigned long)framesSeen,
      (unsigned long)badFramesSeen,
      targetCount,
      ld2450RxPin,
      ld2450TxPin
    );

    if (!hasReceivedFrame && bytesSeen > 0) {
      printRawSampleIfAvailable();
    }
  }

public:
  RadarDevice(int configuredLd2450RxPin, int configuredLd2450TxPin)
    : SecurityDevice("motion", configuredLd2450RxPin, RADAR_MAX_VALUE),
      ld2450RxPin(configuredLd2450RxPin),
      ld2450TxPin(configuredLd2450TxPin) {}

      