#pragma once
#include <ArduinoMqttClient.h>

void initDevices();
void handleMessage(char message[], String topic);
void makeDataReadings(MqttClient mqttClient);
void registerDevices(MqttClient mqttClient);
