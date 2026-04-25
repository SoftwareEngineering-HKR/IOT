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

  void macToString(uint8_t mac[6], char buffer[13]){
    int j = 0;
    for(int i = 0; i < 6; i ++){
      uint8_t f = (mac[i] & 0xF0) >> 4; 
      uint8_t l = mac[i] & 0x0F; 

      if(f <= 9){ buffer[j++] =  '0' + f;}
      else{ buffer[j++] =  'A' + ( f - 10);}

      if( l <= 9){ buffer[j++] =  '0' + l;}
      else{ buffer[j++] = 'A' + ( l - 10);}
    }
    buffer[j] = '\0';
  }

}

Device::Device(const char* type, int pin, int maxVal){
  strncpy(this->type, type, sizeof(this->type));
  this->type[sizeof(this->type)-1] = '\0';
  this->pin = pin;
  this->maxVal = maxVal;
}

void Device::getTopic(char* buffer, size_t size){
  char macBuffer[13];
  macToString(mac, macBuffer);
  snprintf(buffer, size, "%s/%s", type, macBuffer);
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
