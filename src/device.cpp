#include "device.h"

Device::Device(const char* type, int pin, int maxVal){
  strncpy(this->type, type, sizeof(this->type));
  this->type[sizeof(this->type)-1] = '\0';
  this->pin = pin;
  this->maxVal = maxVal;
}

void Device::getTopic(char* buffer, size_t size){
}

void Device::init(){
}

void Device::getRegistration(char* buffer){
}
