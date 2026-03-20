#pragma once
#include <WiFi.h>
#include <DHT.h>
#include "../include/device.h"


extern DHT dhtSensor;
// Device classes
class DHTTempDevice : public Device {
public:
  DHTTempDevice(int pin) : Device("temperature", pin, 50) {} // DHT11 max temp is 50C
  
  void init() override {
    WiFi.macAddress(this->mac); // Inject  Pico W Wi-Fi MAC
  }
  
  int getReading() override {
    float t = dhtSensor.readTemperature();
    return isnan(t) ? -1 : (int)t;
  }
};
class DHTHumDevice : public Device {
public:
  DHTHumDevice(int pin) : Device("humidity", pin, 100) {} // DHT11 max humidity is 100%
  
  void init() override {
    WiFi.macAddress(this->mac); 
  }
  
  int getReading() override {
    float h = dhtSensor.readHumidity();
    return isnan(h) ? -1 : (int)h;
  }
};
class LightDevice : public Device {
public:
  LightDevice(int pin) : Device("light", pin, 100) {} // We'll convert raw ADC to percentage
  
  void init() override {
    WiFi.macAddress(this->mac);
  }
  
  int getReading() override {
    uint16_t raw = analogRead(pin);
    return (int)(((float)raw / 65535.0) * 100.0); //ADC is 16-bit on Pico W
  }
};