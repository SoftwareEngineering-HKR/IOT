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
