#include "devices.h"
#include <Arduino.h>

// generic sensor device
SensorDevice::SensorDevice(int pin, const char* type, int maxVal)
: Device(type, pin, maxVal){};

void SensorDevice::init(){
   Device::init();
   pinMode(pin, INPUT);
}

// generic digital sensor
DigitalSensorDevice::DigitalSensorDevice(int pin, const char* type)
: SensorDevice(pin, type, 1){}

int DigitalSensorDevice::getReading(){ return digitalRead(pin); }

// generic analog sensor
AnalogSensorDevice::AnalogSensorDevice(int pin, const char* type)
: SensorDevice(pin, type, 1023){}

int AnalogSensorDevice::getReading(){ return analogRead(pin); }

// generic reciver device
ReciverDevice::ReciverDevice(int pin, const char* type, void(*proc)(const char*, int))
: Device(type, pin, 1), process(proc){}

void ReciverDevice::init(){
  Device::init();
  pinMode(pin, OUTPUT);
}

void ReciverDevice::handleMessage(const char* msg){
  process(msg, pin);
}
