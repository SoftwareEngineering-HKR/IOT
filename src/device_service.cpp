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
  if(strcmp(msg, "on") == 0){
    digitalWrite(pin, HIGH);
  }else if (strcmp(msg, "off") == 0){
    digitalWrite(pin, LOW);
  }
}



void initDevices(){
} 

void handleMessage(char message[], String topic){
}

void makeDataReadings(MqttClient mqttClient){
}

void registerDevices(MqttClient mqttClient){
}