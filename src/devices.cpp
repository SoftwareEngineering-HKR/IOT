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

// fan class 
FanDevice::FanDevice( int pin1, int pin2)
: Device("fan", pin1, 2), pin2(pin2){}

void FanDevice::init(){
  Device::init();
  pinMode(pin, OUTPUT);
  pinMode(pin2, OUTPUT);
}

void FanDevice::handleMessage(const char *msg){
  if(strcmp(msg, "0") == 0){
    digitalWrite(pin, LOW);
    digitalWrite(pin2, LOW);
  }else if (strcmp(msg, "1") == 0){
    digitalWrite(pin, HIGH);
    digitalWrite(pin2, LOW);
  }else if (strcmp(msg, "2") == 0){
    digitalWrite(pin, LOW);
    digitalWrite(pin2, HIGH);
  }
}

// servo class
ServoDevice::ServoDevice(int pin)
: Device("servo", pin, 1),pos(0){}

void ServoDevice::init(){ 
  Device::init();
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  servo.attach(pin);
}

void ServoDevice::handleMessage(const char *msg){
  if(strcmp(msg, "open") == 0){
      servo.write(180);
  }else if (strcmp(msg, "close") == 0){
      servo.write(0);
  }
}

// lcd screen class
LcdDevice::LcdDevice(int pin)
: Device("display", pin, 32), mylcd(LiquidCrystal_I2C(0x27,16,2)){}

void LcdDevice::init(){ 
  Device::init();
  mylcd.init();
  mylcd.backlight();
}

void LcdDevice::handleMessage(const char *msg){
  mylcd.clear();
  mylcd.setCursor(0,0);
  for(int i = 0; msg[i] != '\0'; i++){
    if (i == 16){
      mylcd.setCursor(0, 1);
    }
    mylcd.print(msg[i]);
  }
}