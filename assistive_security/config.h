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


// -----------------------------------------------------------------------------
// Backend / transport settings
// -----------------------------------------------------------------------------
#define MQTT_BROKER_PORT 1883
#define UDP_DISCOVERY_PORT 4444
#define UDP_DISCOVERY_MESSAGE "NetworkDiscovery;Ver=1;"
#define UDP_DISCOVERY_RESPONSE "DiscoveryResponse;"

// Set to 1 if you want to bypass UDP discovery and use a fixed backend IP.
#define USE_STATIC_SERVER_IP 1
#define STATIC_SERVER_IP_1 192
#define STATIC_SERVER_IP_2 168
#define STATIC_SERVER_IP_3 50
#define STATIC_SERVER_IP_4 130

// -----------------------------------------------------------------------------
// Publish behaviour
// -----------------------------------------------------------------------------
#define SENSOR_PUBLISH_INTERVAL_MS 300UL
#define MQTT_RECONNECT_INTERVAL_MS 5000UL
#define WIFI_RECONNECT_INTERVAL_MS 10000UL
#define DISCOVERY_RETRY_INTERVAL_MS 2000UL
#define RADAR_DEBUG_PRINT_MS 10000UL

// -----------------------------------------------------------------------------
// Sensor ranges used for backend registration
// -----------------------------------------------------------------------------


#define LIDAR_MIN_VALUE 0
#define LIDAR_MAX_VALUE 9000

#define RADAR_MIN_VALUE 0
#define RADAR_MAX_VALUE 3


#define RFID_MIN_VALUE 0
#define RFID_MAX_VALUE 1


