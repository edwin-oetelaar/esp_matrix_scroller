#include <Arduino.h>
#include <LittleFS.h>
#include <debug.h>
#include <font5x8.h>
#include <ledpanel.h>

CRGB leds[NUM_LEDS];

textsettings_t textSettings;
xSemaphoreHandle textSettingsMutex;

int getLedIndex(int x, int y) {
    // skip illegals
    if (x >= WIDTH || x < 0 || y >= HEIGHT || y < 0)
        return -1;

    x = WIDTH-1-x;
    y = HEIGHT-1-y;

    // Odd row
    if (x & 0x01) {
        return ((x+1)*HEIGHT)-1-y;
    }

    return (x*HEIGHT)+y;
}

int getTextWidth(char* text) {
    return strlen(text)*(FONT_WIDTH+FONT_SPACING);
}

void printText(char* text, int xpos, CRGB fcolor, CRGB bcolor, bool transparent) {
    // for each char
    for (int c=0;c<strlen(text);c++) {
        unsigned char *symbol = (unsigned char*)font[text[c]-0x20];

        // for each col
        for (int col=0;col<FONT_WIDTH;col++) {
            unsigned char sbyte = symbol[col];

            for (int bit=0;bit<FONT_HEIGHT;bit++) {
                int b = bit;
                int x = xpos + col + c*(FONT_WIDTH+FONT_SPACING);
                int index = getLedIndex(x, b);
                // skip invalid index
                if (index == -1) {
                    continue;
                }
                // bg fg color
                if (sbyte & 1<<b) {
                    leds[index] = fcolor;
                }
                else if (!transparent) {
                    leds[index] = bcolor;
                }
            }
        }
    }
}

void printText(char* text, int xpos, CRGB fcolor, CRGB bcolor) {
    printText(text, xpos, fcolor, bcolor, false);
}

void setText(textsettings_t *settings, char *text) {
    delete[] settings->text;
    settings->text = new char[strlen(text)+1];
    strcpy(settings->text, text);
}

void setText(textsettings_t *settings, const String &text) {
    delete[] settings->text;
    settings->text = new char[text.length()+1];
    strcpy(settings->text, text.c_str());
}

void writeConfigToFlash(textsettings_t *settings) {
    File config = LittleFS.open("/config", "w");

    uint32_t text_length = strlen(settings->text);
    config.write((uint8_t *)&text_length, sizeof(text_length));
    config.write((uint8_t *)settings->text, text_length);
    config.write(&settings->fcolor.r, 1);
    config.write(&settings->fcolor.g, 1);
    config.write(&settings->fcolor.b, 1);
    config.write(&settings->bcolor.r, 1);
    config.write(&settings->bcolor.g, 1);
    config.write(&settings->bcolor.b, 1);
    config.write((uint8_t *)&settings->speed, 2);

    config.close();
}

void readConfigFromFlash(textsettings_t *settings) {
    File config = LittleFS.open("/config", "r");

    uint32_t text_length = 0;
    config.read((uint8_t *)&text_length, sizeof(text_length));

    char *text = new char[text_length+1];
    config.read((uint8_t*)text, text_length);
    text[text_length] = '\0';

    CRGB fcolor;
    CRGB bcolor;
    config.read(&fcolor.r, 1);
    config.read(&fcolor.g, 1);
    config.read(&fcolor.b, 1);
    config.read(&bcolor.r, 1);
    config.read(&bcolor.g, 1);
    config.read(&bcolor.b, 1);

    uint16_t speed = 0;
    config.read((uint8_t *)&speed, sizeof(speed));

    settings->text = nullptr;
    setText(settings, text);
    settings->fcolor = fcolor;
    settings->bcolor = bcolor;
    settings->speed = speed;

    delete[] text;
    config.close();
}

void LedTask(void *pvParameters) {
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

    xSemaphoreTake(textSettingsMutex, portMAX_DELAY);
    // Load initial settings from disk
    readConfigFromFlash(&textSettings);
    textSettings.changed = true;
    xSemaphoreGive(textSettingsMutex);

    bool firstboot = true;

    int text_width;
    int overflow;
    int xpos;
    bool scrolling;

    char *text = nullptr;
    CRGB fcolor;
    CRGB bcolor;
    int speed;

    while(true) {
        xSemaphoreTake(textSettingsMutex, portMAX_DELAY);
        if (textSettings.changed) {
            // copy out vals
            delete[] text;
            text = new char[strlen(textSettings.text)+1];
            strcpy(text, textSettings.text);

            speed = textSettings.speed;
            fcolor = textSettings.fcolor;
            bcolor = textSettings.bcolor;

            text_width = getTextWidth(text);
            overflow = text_width - WIDTH;
            xpos = 0;

            scrolling = false;
            if (overflow > 0) {
                scrolling = true;
            }

            // skip rewriting flash on first boot
            if (!firstboot) {
                writeConfigToFlash(&textSettings);
            }

            firstboot = false;
            textSettings.changed = false;
        }
        xSemaphoreGive(textSettingsMutex);

        if (scrolling) {
            fill_solid(leds, NUM_LEDS, bcolor);

            int newx = xpos + text_width + FONT_WIDTH + FONT_SPACING;

            if (newx < WIDTH) {
                printText(text, newx, fcolor, CRGB::Black, true);
            }

            if (xpos + text_width >= 0) {
                printText(text, xpos, fcolor, CRGB::Black, true);
            }

            xpos--;

            if (newx <= 0) {
                xpos = newx-1;
            }

            FastLED.show(100);
            vTaskDelay(pdMS_TO_TICKS(MAX_SPEED+MIN_SPEED-speed));
        }
        else {
            fill_solid(leds, NUM_LEDS, bcolor);
            printText(text, 0, fcolor, CRGB::Black, true);
            FastLED.show(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

int panelSetup() {
    textSettingsMutex = xSemaphoreCreateMutex();
    // Create LED task
    xTaskCreatePinnedToCore(LedTask, "LedTask", 10000, NULL, 1, NULL,  1);
    return 0;
}