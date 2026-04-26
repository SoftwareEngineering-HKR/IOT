#pragma once

// -----------------------------------------------------------------------------
// Wi-Fi configuration
// -----------------------------------------------------------------------------
#define WIFI_SSID "zenwifi"
#define WIFI_PASSWORD "Ubelive783188*"

// -----------------------------------------------------------------------------
// Optional webhook / legacy values kept for compatibility with the wider project
// -----------------------------------------------------------------------------
#define DISCORD_WEBHOOK_URL "https://discord.com/api/webhooks/*****"
#define TEMP_THRESHOLD 30
#define HUMIDITY_THRESHOLD 80
#define LIGHT_THRESHOLD 20
#define DHT_PIN 16
#define ADC_PIN 27
#define TILT_PIN 12
#define LED_PIN 13

// -----------------------------------------------------------------------------
// ESP32-S3 Super Mini hardware mapping
// RC522 and VL53L0X share the same I2C bus
// -----------------------------------------------------------------------------
#define BOARD_LED_PIN 8

#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 6