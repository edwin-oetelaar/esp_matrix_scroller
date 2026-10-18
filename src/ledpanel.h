#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <FastLED.h>

#define WIDTH 128
#define HEIGHT 8

#define NUM_LEDS WIDTH*HEIGHT
#define DATA_PIN 33

// LET OP: nieuwe hardwareversie! Een MOSFET op GPIO 21 schakelt de voeding
// naar de ledstrip; deze pin moet HOOG zijn, anders blijft de strip donker.
// De vorige hardwareversie had deze MOSFET niet (strip altijd gevoed).
// Zie HARDWARE.md.
#define LED_POWER_PIN 21

// power budget for the panel, FastLED dims the frame to stay below it
// full white on all leds at brightness 100 is ~24A, so keep this in check
#define LED_VOLTS 5
#define LED_MAX_MILLIAMPS 10000

#define MIN_SPEED 10
#define MAX_SPEED 255

typedef struct _textsettings_t {
    char *text;
    CRGB fcolor;
    CRGB bcolor;
    uint32_t speed; // 10 - 255
    bool changed;
} textsettings_t;

extern textsettings_t textSettings;
extern xSemaphoreHandle textSettingsMutex;

int panelSetup();
int getLedIndex(int x, int y);
void setPixel(int x, int y, CRGB color);
void printText(const char* text, int xpos, CRGB fcolor, CRGB bcolor, bool transparent);
void printText(const char* text, int xpos, CRGB fcolor, CRGB bcolor);
void setText(textsettings_t *settings, const char *text);
void setText(textsettings_t *settings, const String &text);