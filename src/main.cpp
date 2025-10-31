#include <Arduino.h>
#include <LittleFS.h>

#define FASTLED_USES_ESP32S3_I2S true

#include <debug.h>
#include <networking.h>
#include <ledpanel.h>

void setup() {

#ifdef DEBUG
    Serial.begin(115200);
#endif

    DEBUG_PRINTLN("Init");

    LittleFS.begin(false);

    panelSetup();

    networkSetup();
}

void loop() {
    networkTick();
}