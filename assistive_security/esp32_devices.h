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
MFRC522 rfid(RC522_I2C_ADDRESS, RC522_RST_PIN);   // RC522_RST_PIN may remain -1, 


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
