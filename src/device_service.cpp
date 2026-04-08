#include "device_service.h"
#include "devices.h"
#include <Arduino.h>

void playMusic(const char* msg, int pin) {
  for (int i = 0; msg[i] != '\0'; i++){
    for(int j = 0; j < msg[i]; j++){
      digitalWrite(pin, HIGH);
      delay(2);
      digitalWrite(pin, LOW);
      delay(2);
    }
    delay(100);
  }
};

void binaryReciver(const char* msg, int pin){
  if(strcmp(msg, "1") == 0){
    digitalWrite(pin, HIGH);
  }else if (strcmp(msg, "0") == 0){
    digitalWrite(pin, LOW);
  }
}

DigitalSensorDevice motion( 2, "motion");
DigitalSensorDevice button1(4, "button");
DigitalSensorDevice button2( 8, "button");

AnalogSensorDevice gas( A0, "gas");
AnalogSensorDevice photo( A1, "photo");
AnalogSensorDevice steam( A2, "steam");
AnalogSensorDevice humidity( A3, "humidity");

ReciverDevice buzzer( 3, "buzz", playMusic);
ReciverDevice light1( 13, "light", binaryReciver);
ReciverDevice light2( 5, "light", binaryReciver);

FanDevice fan( 6, 7);
ServoDevice window( 10);
ServoDevice door( 9);
LcdDevice screen( -1);

Device* devices[] = {&light1, &light2, &buzzer, &fan, &window, &door, &screen, &photo, &motion, &steam, &humidity, &gas, &button1, &button2};

const int deviceCount = sizeof(devices)/sizeof(devices[0]);

char buffer[64];

void handleDeviceCommand(char message[], String topic){
  char buffer[21];
  for (int i = 0; i < deviceCount; i++) {
    devices[i]->getTopic(buffer, sizeof(buffer));
    if(topic.equals(buffer)){
      devices[i]->handleMessage(message);
      break;
    }
  }
}

int handleIncoming(){
  char* topic = strtok(buffer, ":");
  char* message = strtok(NULL, ":");

  if(strcmp(topic, "cmd") == 0){
    registerDevices();
    return 1;
  }else{
    handleDeviceCommand(message, topic);
    return 0;
  }
}

void readSerial(){
  int len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
  buffer[len] = '\0'; 
}

int awaitAck() {

  while(true){
    readSerial();
    if (strcmp(buffer, "ACK") == 0){
      return 0;
    }
    if(handleIncoming() == 1){
      return 1;
    }
  }
}


void handleMessage(){
  readSerial();
  handleIncoming();
}


void makeDataReadings(){
  char topic[22];
  char buffer[128];
  for (int i = 0; i < deviceCount; i++) {
    int data = devices[i]->getReading();
    if(data != -1) {
      devices[i]->getTopic(topic, sizeof(topic));
      int n = snprintf(buffer, sizeof(buffer), "%s:%d\n", topic, data);
      Serial.write(buffer, n);
      if (awaitAck() == 1){
        return;
      } 
    }
  }
}

void subDevice(Device* device, char* buffer) {
  if (device->getReading() == -1) {
    device->getTopic(buffer, 21);
    Serial.println(buffer);
  }
}

void registerDevices(){
  char topic[21];

  for (int i = 0; i < deviceCount; i++) {
    devices[i]->getRegistration(Serial);
    awaitAck();
    subDevice(devices[i], topic);
  }
  Serial.println("DONE");
  awaitAck();
}

void initDevices(){
  for (int i = 0; i < deviceCount; i++) {
    devices[i]->init();
  }
}