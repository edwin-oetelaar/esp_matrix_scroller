#include <Arduino.h>
#include <LittleFS.h>

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

    pinMode(35, OUTPUT);
}

uint8_t heartbeat[8] = {0, 0, 0, 1, 0, 1, 0, 0};
int heartbeat_off = 0;
int oldMillis = 0;

void loop() {
    networkTick();

    if (millis() - oldMillis > 100) {
        digitalWrite(35, heartbeat[heartbeat_off]);
        heartbeat_off++;
        if (heartbeat_off >= sizeof(heartbeat)) {
            heartbeat_off = 0;
        }
        oldMillis = millis();
    }
}