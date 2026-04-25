#pragma once
#include <stdint.h>
#include <string.h>

class Device{
  protected:
    uint8_t mac[6]; // unique device id
    char type[10]; // device type
    int maxVal; // the maximum value the device state can be 
    int pin; // sensor pin

  public:
    Device(const char* type, int pin, int maxVal); // constructor
    
    void getTopic(char* buffer, size_t size); // get the topic for the device
    void getRegistration(char* buffer); // the registration pay load

    virtual void handleMessage(const char* msg) {}; // handle mqtt message 
    virtual int getReading() {return -1;}; // get readin from sensor
    virtual void init() = 0; // initalises the device when hardware is ready
};