#pragma once
#include <Arduino.h>

class BluetoothService {
public:
  void init();
  void loop();

  bool connected();

  void sendRegister(const char* json);
  void sendPing();
  void sendValue(const char* topic, const char* msg);

  // don't know if this is needed, needs to be tested
  // void sendToRemote(const char* msg); 

  void onMessage(char* msg) {}
};
