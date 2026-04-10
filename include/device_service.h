#pragma once
#include <ArduinoMqttClient.h>

void initDevices();
void handleMessage(char message[], String topic);
void makeDataReadings(MqttClient mqttClient);
void registerDevices(MqttClient mqttClient);
void registerDevicesBluetooth(BluetoothService& bt);
void handleMessageBluetooth(char message[], String topic);
void makeDataReadingsBluetooth(BluetoothService& bt);
