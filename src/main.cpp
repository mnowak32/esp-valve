#include <Arduino.h>
#include <ButtonDebounce.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

#include <WifiManager.h>
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>

#include "config.h"

#define BUT_OPEN_PIN D5
#define BUT_CLOSE_PIN D6
#define OUT_OPEN_PIN D2
#define OUT_CLOSE_PIN D1
#define OUT_LED_PIN D7

#define BLYNK_LED_OPEN V1
#define BLYNK_LED_CLOSE V2
#define BLYNK_LED_STATUS V0

#define BLYNK_BUT_OPEN V3
#define BLYNK_BUT_CLOSE V4

#define DEBOUNCE_TIME 125

enum State {
    VLV_PREBOOT, VLV_BOOT, VLV_SETUP, VLV_OPENING, VLV_OPENED, VLV_CLOSING, VLV_CLOSED, VLV_ERROR
};

enum Command {
    CMD_NONE, CMD_OPEN, CMD_CLOSE
};

#define PREBOOT_LENGTH 500
#define BOOT_BUTTON_WAIT 5000

#define RELAY_OPEN_TIME 6000

ButtonDebounce butOpen(BUT_OPEN_PIN, DEBOUNCE_TIME),
    butClose(BUT_CLOSE_PIN, DEBOUNCE_TIME);

State appState = VLV_PREBOOT;
Command cmd = CMD_NONE;

uint32_t timestamp = 0, now = 0, ledVal = 0;

#define WIFI_AP_SSID "Boring AP"
#define WIFI_AP_PASS "defaultPass"

WidgetLED ledOpen(BLYNK_LED_OPEN),
    ledClose(BLYNK_LED_CLOSE),
    ledStatus(BLYNK_LED_STATUS);


void setup() {
    WiFiManager wifiMan;

    timestamp = millis();
    pinMode(BUT_OPEN_PIN, INPUT_PULLUP);
    pinMode(BUT_CLOSE_PIN, INPUT_PULLUP);
    pinMode(OUT_OPEN_PIN, OUTPUT);
    pinMode(OUT_CLOSE_PIN, OUTPUT);
    pinMode(OUT_LED_PIN, OUTPUT);
    analogWriteRange(1023);

    Serial.begin(115200);
    wifiMan.autoConnect(WIFI_AP_SSID, WIFI_AP_PASS);

    Blynk.config(BLYNK_TOKEN);
    while(Blynk.connect() != true) {}
    ledStatus.off();
    ledOpen.off();
    ledClose.off();
}

BLYNK_WRITE(BLYNK_BUT_OPEN) {
    int value = param.asInt();
    if (appState == VLV_CLOSED && value == 1) {
        cmd = CMD_OPEN;
    }
}

BLYNK_WRITE(BLYNK_BUT_CLOSE) {
    int value = param.asInt();
    if (appState == VLV_OPENED && value == 1) {
        cmd = CMD_CLOSE;
    }
}

void loop() {
    now = millis();
    butClose.update();
    butOpen.update();
    switch(appState) {
        case VLV_PREBOOT: //wait for eventual buttons debounce
            if ((now - timestamp) >= PREBOOT_LENGTH) {
                timestamp = now;
                appState = VLV_BOOT;
            }
        case VLV_BOOT: //check if close button was pressed for at least 5 sec., if it was enter setup state
            if (butClose.state() == LOW) {
                if ((now - timestamp) >= BOOT_BUTTON_WAIT) {
                    appState = VLV_SETUP;
                }
            } else {
                appState = VLV_CLOSING;
                ledStatus.on();
            }
            ledVal = 256;
            break;
        case VLV_SETUP: {
                WiFiManager wifiMan;
                Serial.println("Entering setup on demand");
                wifiMan.startConfigPortal(WIFI_AP_SSID, WIFI_AP_PASS);
                Serial.println("After setup... (resetting)");
                ESP.reset();
            }
            break;
        case VLV_OPENED:
        if (butClose.state() == LOW || cmd == CMD_CLOSE) {
                cmd = CMD_NONE;
                timestamp = now;
                appState = VLV_CLOSING;
                ledClose.on();
            }
            ledVal = ((now / 2048) % 2) ? (now % 2048 / 2) : 1023 - (now % 2048 / 2);
            break;
        case VLV_CLOSED:
            if (butOpen.state() == LOW || cmd == CMD_OPEN) {
                cmd = CMD_NONE;
                timestamp = now;
                appState = VLV_OPENING;
                ledOpen.on();
            }
            ledVal = ((now / 1024) % 2) ? (now % 1024) : 1023 - (now % 1024);
            break;
        case VLV_OPENING:
            if ((now - timestamp) >= RELAY_OPEN_TIME) {
                appState = VLV_OPENED;
                digitalWrite(OUT_OPEN_PIN, 0);
                ledOpen.off();
            } else {
                digitalWrite(OUT_OPEN_PIN, 1);
            }
            ledVal = (now / 125) % 2 * 512;
            break;
        case VLV_CLOSING:
            if ((now - timestamp) >= RELAY_OPEN_TIME) {
                appState = VLV_CLOSED;
                digitalWrite(OUT_CLOSE_PIN, 0);
                ledClose.off();
            } else {
                digitalWrite(OUT_CLOSE_PIN, 1);
            }
            ledVal = (now / 250) % 2 * 512;
            break;
    }
    Blynk.run();
    analogWrite(OUT_LED_PIN, ledVal);
    delay(2);
}
