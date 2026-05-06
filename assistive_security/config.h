#pragma once

// -----------------------------------------------------------------------------
// Wi-Fi configuration
// -----------------------------------------------------------------------------
#define WIFI_SSID "*****"
#define WIFI_PASSWORD "*******"


// -----------------------------------------------------------------------------
// ESP32-S3 Super Mini hardware mapping
// RC522 and VL53L0X share the same I2C bus
// -----------------------------------------------------------------------------
#define BOARD_LED_PIN 8

#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 6

#define LIDAR_SDA_PIN I2C_SDA_PIN
#define LIDAR_SCL_PIN I2C_SCL_PIN


#define RC522_SDA_PIN I2C_SDA_PIN
#define RC522_SCL_PIN I2C_SCL_PIN
#define RC522_I2C_ADDRESS 0x28