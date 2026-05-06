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


#define AUTHORIZED_RFID_UID "EA:A3:75:DC"



// IMPORTANT:
// The MFRC522 I2C library expects a real reset GPIO when PCD_Init() is called.
// If your RC522 RST pin is NOT wired to the ESP32, keep this at -1 and RFID will
// be skipped safely instead of throwing an invalid-pin error at boot.
#define RC522_RST_PIN -1

#define ENABLE_RFID 1

#define LD2450_TX_PIN 1
#define LD2450_RX_PIN 2
#define LD2450_BAUD_RATE 256000

