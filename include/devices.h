#pragma once
#include "device.h"
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

class SensorDevice: public Device{
  public:
    SensorDevice( int pin, const char* type, int maxVal);
    void init() override;
};

class DigitalSensorDevice: public SensorDevice{
  public:
    DigitalSensorDevice(int pin, const char* type);
    int getReading() override;
};

class AnalogSensorDevice: public SensorDevice{
  public:
    AnalogSensorDevice( int pin, const char* type);
    int getReading() override;
};

class ReciverDevice: public Device{
    void (*process)(const char* msg, int pin);
  public:
    ReciverDevice( int pin, const char* type, void(*proc)(const char*, int));
    void handleMessage(const char* msg) override;
    void init() override;
};

class FanDevice: public Device {
    int pin2;
  public:
    FanDevice( int pin1, int pin2);
    void handleMessage(const char* msg) override;
    void init() override;
};

class ServoDevice: public Device {
    Servo servo;
    int pos;
  public:
    ServoDevice( int pin1);
    void handleMessage(const char* msg) override;
    void init() override;
};

class LcdDevice: public Device {
    LiquidCrystal_I2C mylcd;
  public:
    LcdDevice(int pin);
    void handleMessage(const char* msg) override;
    void init() override;
};