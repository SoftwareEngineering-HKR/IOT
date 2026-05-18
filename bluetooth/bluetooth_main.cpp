#include <Arduino.h>
#include "bluetooth_service.h"

BluetoothService ble;

void setup() {

    Serial.begin(115200);

    ble.init();

    Serial.println("[BLE] Ready");
}

void loop() {

    ble.loop();

    delay(10);
}
