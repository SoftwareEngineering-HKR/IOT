#include <Arduino.h>
#include "device_service.h"

unsigned long lastSend = 0;

unsigned int Interval = 5000;
int Baut = 9600;
int randSeed = 123;

void setup() {
  Serial.begin(Baut);
  Serial.setTimeout(1000);
  randomSeed(randSeed);
  initDevices();
  Serial.println("RDY");
}

void loop() {
  if (Serial.available()){
    handleMessage();
  }

  unsigned long now = millis();
  if (now - lastSend > Interval){
    lastSend = now;
    makeDataReadings();
  }
}