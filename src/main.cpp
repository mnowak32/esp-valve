#include <Arduino.h>
#include <ButtonDebounce.h>

#define BUT_OPEN_PIN D6
#define BUT_CLOSE_PIN D5
#define OUT_OPEN_PIN D2
#define OUT_CLOSE_PIN D1
#define OUT_LED_PIN D4

#define DEBOUNCE_TIME 125

enum State {
    PREBOOT, BOOT, SETUP, OPENING, OPENED, CLOSING, CLOSED, ERROR
};

#define PREBOOT_LENGTH 500
#define BOOT_BUTTON_WAIT 5000

#define RELAY_OPEN_TIME 6000

ButtonDebounce butOpen(BUT_OPEN_PIN, DEBOUNCE_TIME),
    butClose(BUT_CLOSE_PIN, DEBOUNCE_TIME);

State appState = PREBOOT;
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
        case PREBOOT: //wait for eventual buttons debounce
            if ((now - timestamp) >= PREBOOT_LENGTH) {
                timestamp = now;
                appState = BOOT;
            }
        case BOOT: //check if close button was pressed for at least 5 sec., if it was enter setup state
            if (butClose.state() == LOW) {
                if ((now - timestamp) >= BOOT_BUTTON_WAIT) {
                    appState = SETUP;
                }
            } else {
                appState = CLOSING;
            }
            ledVal = 256;
            break;
        case SETUP:
            appState = CLOSING; //temporary
            ledVal = 1024;
            break;
        case OPENED:
            if (butClose.state() == LOW) {
                timestamp = now;
                appState = CLOSING;
            }
            ledVal = ((now / 2048) % 2) ? (now % 2048 / 2) : 1023 - (now % 2048 / 2);
            break;
        case CLOSED:
            if (butOpen.state() == LOW) {
                timestamp = now;
                appState = OPENING;
            }
            ledVal = ((now / 1024) % 2) ? (now % 1024) : 1023 - (now % 1024);
            break;
        case OPENING:
            if ((now - timestamp) >= RELAY_OPEN_TIME) {
                appState = OPENED;
                digitalWrite(OUT_OPEN_PIN, 0);
            } else {
                digitalWrite(OUT_OPEN_PIN, 1);
            }
            ledVal = (now / 125) % 2 * 512;
            break;
        case CLOSING:
            if ((now - timestamp) >= RELAY_OPEN_TIME) {
                appState = CLOSED;
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