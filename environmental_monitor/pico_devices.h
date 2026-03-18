#pragma once
#include <WiFi.h>
#include <DHT.h>
#include "../include/device.h"


extern DHT dhtSensor;
// Device classes
class DHTTempDevice : public Device {
public:
  DHTTempDevice(int pin) : Device("temperature", pin, 50) {}
  
  void init() override {
    WiFi.macAddress(this->mac); // Inject  Pico W Wi-Fi MAC
  }
  
  int getReading() override {
    float t = dhtSensor.readTemperature();
    return isnan(t) ? -1 : (int)t;
  }
};