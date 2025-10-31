#include <Arduino.h>
#include <debug.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <networking.h>
#include <ledpanel.h>

// Replace with your network credentials
const char* ssid_prefix = "Matrix ";
const char* password    = "123456789";

WiFiServer server(80);
DNSServer dnsServer;
const IPAddress localIP(4, 3, 2, 1);
const String localIPURL = "http://4.3.2.1";
const IPAddress gatewayIP(4, 3, 2, 1);
const IPAddress subnetMask(255, 255, 255, 0);

// Variable to store the HTTP request
String header;
String pagebuf;

uint8_t readHexByte(char *c) {
    char hi = c[0];
    char lo = c[1];

    hi = toupper(hi);
    lo = toupper(lo);

    if (hi >= '0' && hi <= '9') {
        hi = hi-'0';
    }
    else {
        hi = hi-'A'+10;
    }

    if (lo >= '0' && lo <= '9') {
        lo = lo-'0';
    }
    else {
        lo = lo-'A'+10;
    }

    return (hi<<4) | lo;
}

void handlePostData(const String &body) {
    String data = body;

    int count = 0;
    String text;
    int speed;
    CRGB fcolor;
    CRGB bcolor;

    int start = 0;

    while (start < data.length()) {
        int eq = data.indexOf('=', start);
        int amp = data.indexOf('&', start);
        if (eq == -1) break;
        if (amp == -1) amp = data.length();

        String key = data.substring(start, eq);
        String value = data.substring(eq + 1, amp);

        DEBUG_PRINT("Param: ");
        DEBUG_PRINT(key);
        DEBUG_PRINT(" = ");
        DEBUG_PRINTLN(value);

        if (key == "text") {
            text = value;
            text.replace("+", " ");
            text.replace("%20", " ");
            text.replace("%21", "!");
            text.replace("%22", "\"");
            text.replace("%23", "#");
            text.replace("%24", "$");
            text.replace("%25", "%");
            text.replace("%26", "&");
            text.replace("%27", "'");
            text.replace("%28", "(");
            text.replace("%29", ")");
            text.replace("%2A", "*");
            text.replace("%2B", "+");
            text.replace("%2C", ",");
            text.replace("%2D", "-");
            text.replace("%2E", ".");
            text.replace("%2F", "/");
            text.replace("%3A", ":");
            text.replace("%3B", ";");
            text.replace("%3C", "<");
            text.replace("%3D", "=");
            text.replace("%3E", ">");
            text.replace("%3F", "?");
            text.replace("%40", "@");
            text.replace("%5B", "[");
            text.replace("%5C", "\\");
            text.replace("%5D", "]");
            text.replace("%5E", "^");
            text.replace("%5F", "_");
            text.replace("%60", "`");
            text.replace("%7B", "{");
            text.replace("%7C", "|");
            text.replace("%7D", "}");
            text.replace("%7E", "~");
            count++;
        }
        if (key == "speed") {
            speed = atoi(value.c_str());
            count++;
        }
        else if (key == "fcolor") {
            fcolor.r = readHexByte((char *)value.c_str()+3);
            fcolor.g = readHexByte((char *)value.c_str()+5);
            fcolor.b = readHexByte((char *)value.c_str()+7);
            count++;
        }
        else if (key == "bcolor") {
            bcolor.r = readHexByte((char *)value.c_str()+3);
            bcolor.g = readHexByte((char *)value.c_str()+5);
            bcolor.b = readHexByte((char *)value.c_str()+7);
            count++;
        }

        start = amp + 1;
    }

    if (count >= 4) {
        xSemaphoreTake(textSettingsMutex, portMAX_DELAY);
        setText(&textSettings, text);
        textSettings.speed = speed;
        textSettings.fcolor = fcolor;
        textSettings.bcolor = bcolor;
        textSettings.changed = true;
        xSemaphoreGive(textSettingsMutex);
    }
}

void networkTick() {
    WiFiClient client = server.accept();
    if (client) {
        DEBUG_PRINTLN("New Client.");
        String currentLine = "";

        bool isPost = false;
        int contentLength = 0;

        // if the client is connected and data is available, read it out
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                header += c;

                // Detect end of headers
                if (header.endsWith("\r\n\r\n")) {

                    // Detect if POST
                    if (header.startsWith("POST ")) {
                        isPost = true;
                    }

                    // Get Content-Length (POST body size)
                    int index = header.indexOf("Content-Length:");
                    if (index != -1) {
                        contentLength = header.substring(index + 15).toInt();
                    }

                    break;  // headers done
                }
            }
        }
        String body = "";
        if (isPost && contentLength > 0) {
            unsigned long start = millis();
            while (body.length() < contentLength && millis() - start < 2000) {
                if (client.available()) {
                    body += (char)client.read();
                }
            }
            DEBUG_PRINTLN("POST BODY:");
            DEBUG_PRINTLN(body);

            // Now parse POST parameters
            handlePostData(body);
        }

        //copy settings
        xSemaphoreTake(textSettingsMutex, portMAX_DELAY);
        CRGB fcolor = textSettings.fcolor;
        CRGB bcolor = textSettings.bcolor;
        char *text = new char[strlen(textSettings.text)+1];
        strcpy(text, textSettings.text);
        int speed = textSettings.speed;
        xSemaphoreGive(textSettingsMutex);

        String newpage = pagebuf;
        newpage.replace("$TEXT$", text);

        char strbuf[20];
        sprintf(strbuf, "%d", speed);
        newpage.replace("$SPEED$", strbuf);

        sprintf(strbuf, "#%02x%02x%02x", fcolor.r, fcolor.g, fcolor.b);
        newpage.replace("$FCOLOR$", strbuf);
        sprintf(strbuf, "#%02x%02x%02x", bcolor.r, bcolor.g, bcolor.b);
        newpage.replace("$BCOLOR$", strbuf);

        delete[] text;

        // Always respond with page
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println("Connection: close");
        client.println();
        client.println(newpage);
        client.println();

        client.stop();
        DEBUG_PRINTLN("Client disconnected.");
        header = "";
    }
    dnsServer.processNextRequest();
}

void networkSetup() {
    // load page from flash and store it in memory
    File page = LittleFS.open("/page.html");
    int pagesize = page.size();

    char *buf = new char[pagesize+1];
    page.read((uint8_t*)buf, pagesize);
    buf[pagesize] = 0;
    page.close();

    pagebuf = String(buf);

    // free up
    delete[] buf;

    uint8_t mac_addr[8];
    esp_efuse_mac_get_default(mac_addr);
    char *ssid = new char[strlen(ssid_prefix) + 8];
    sprintf(ssid, "%s %02X%02X%02X", ssid_prefix, mac_addr[3], mac_addr[3], mac_addr[5]);

    // Create WiFi
    WiFi.softAPConfig(localIP, gatewayIP, subnetMask);
    WiFi.softAP(ssid, password);
    dnsServer.setTTL(3600);
    dnsServer.start(53, "*", localIP);
    server.begin();
}