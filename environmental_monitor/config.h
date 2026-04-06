#ifndef CONFIG_H
#define CONFIG_H

// Wi-Fi credentials
#define WIFI_SSID "Emperor"
#define WIFI_PASSWORD ""

// Discord Webhook
#define DISCORD_WEBHOOK_URL "https://discord.com/api/webhooks/*****"

// Thresholds
#define TEMP_THRESHOLD 30
#define HUMIDITY_THRESHOLD 80
#define LIGHT_THRESHOLD 20

// GPIO Pins
#define DHT_PIN 16
#define ADC_PIN 27   // Photoresistor (ADC1)
#define TILT_PIN 12
#define LED_PIN 13

#endif

#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef1"
