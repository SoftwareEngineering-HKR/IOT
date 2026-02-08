#pragma once
#include "device.h"

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
