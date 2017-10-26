#include <Arduino.h>
#include <ButtonDebounce.h>
#include <ESP8266WiFi.h>

#define BUT_OPEN_PIN D6
#define BUT_CLOSE_PIN D5
#define OUT_OPEN_PIN D2
#define OUT_CLOSE_PIN D1
#define OUT_LED_PIN D4

#define DEBOUNCE_TIME 125

enum State {
    VLV_PREBOOT, VLV_BOOT, VLV_SETUP, VLV_OPENING, VLV_OPENED, VLV_CLOSING, VLV_CLOSED, VLV_ERROR
};

#define PREBOOT_LENGTH 500
#define BOOT_BUTTON_WAIT 5000

#define RELAY_OPEN_TIME 6000

ButtonDebounce butOpen(BUT_OPEN_PIN, DEBOUNCE_TIME),
    butClose(BUT_CLOSE_PIN, DEBOUNCE_TIME);

State appState = VLV_PREBOOT;
uint32_t timestamp = 0, now = 0, ledVal = 0;

void setup() {
    timestamp = millis();
    pinMode(BUT_OPEN_PIN, INPUT_PULLUP);
    pinMode(BUT_CLOSE_PIN, INPUT_PULLUP);
    pinMode(OUT_OPEN_PIN, OUTPUT);
    pinMode(OUT_CLOSE_PIN, OUTPUT);
    pinMode(OUT_LED_PIN, OUTPUT);
    analogWriteRange(1023);
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
            }
            ledVal = 256;
            break;
        case VLV_SETUP:
            appState = VLV_CLOSING; //temporary
            ledVal = 1024;
            break;
        case VLV_OPENED:
            if (butClose.state() == LOW) {
                timestamp = now;
                appState = VLV_CLOSING;
            }
            ledVal = ((now / 2048) % 2) ? (now % 2048 / 2) : 1023 - (now % 2048 / 2);
            break;
        case VLV_CLOSED:
            if (butOpen.state() == LOW) {
                timestamp = now;
                appState = VLV_OPENING;
            }
            ledVal = ((now / 1024) % 2) ? (now % 1024) : 1023 - (now % 1024);
            break;
        case VLV_OPENING:
            if ((now - timestamp) >= RELAY_OPEN_TIME) {
                appState = VLV_OPENED;
                digitalWrite(OUT_OPEN_PIN, 0);
            } else {
                digitalWrite(OUT_OPEN_PIN, 1);
            }
            ledVal = (now / 125) % 2 * 512;
            break;
        case VLV_CLOSING:
            if ((now - timestamp) >= RELAY_OPEN_TIME) {
                appState = VLV_CLOSED;
                digitalWrite(OUT_CLOSE_PIN, 0);
            } else {
                digitalWrite(OUT_CLOSE_PIN, 1);
            }
            ledVal = (now / 250) % 2 * 512;
            break;
    }
    analogWrite(OUT_LED_PIN, ledVal);
    delay(5);
}