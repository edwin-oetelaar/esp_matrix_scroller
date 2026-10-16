#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <FastLED.h>

#define WIDTH 128
#define HEIGHT 8

#define NUM_LEDS WIDTH*HEIGHT
#define DATA_PIN 33

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
int getLedIndex(uint32_t x, uint32_t y);
void setPixel(uint32_t x, uint32_t y, CRGB color);
void printText(char* text, int xpos, CRGB fcolor, CRGB bcolor, bool transparent);
void printText(char* text, int xpos, CRGB fcolor, CRGB bcolor);
void setText(textsettings_t *settings, char *text);
void setText(textsettings_t *settings, const String &text);