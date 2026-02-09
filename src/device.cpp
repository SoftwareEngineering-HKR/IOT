#include "device.h"
#include <Arduino.h>
#include <ArduinoJson.h>

namespace{
  void genFakeMac(uint8_t mac[6]){
    mac[0] = 0x02;
    for (int i = 1; i < 6; i++){
      mac[i] = random(0, 256);
    }
  }
}

Device::Device(const char* type, int pin, int maxVal){
  strncpy(this->type, type, sizeof(this->type));
  this->type[sizeof(this->type)-1] = '\0';
  this->pin = pin;
  this->maxVal = maxVal;
}

void Device::getTopic(char* buffer, size_t size){
}

void Device::init(){
  genFakeMac(this->mac);
}

void Device::getRegistration(char* buffer){
  int val = this->getReading();
  bool sensor = val != -1;
  StaticJsonDocument<128> doc;
  doc["id"] = mac;
  doc["type"] = type;
  doc["maxVal"] = maxVal;
  doc["sensor"] = sensor;
  serializeJson(doc, buffer, sizeof(buffer));
}
