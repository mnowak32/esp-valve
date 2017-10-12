#include <Arduino.h>
#include <ButtonDebounce.h>

#define BUT_OPEN_PIN D1
#define BUT_CLOSE_PIN D2
#define DEBOUNCE_TIME 125

enum State {
    PREBOOT, BOOT, SETUP, LOOP, OPENING, CLOSING, ERROR
};

#define PREBOOT_LENGTH 500
#define BOOT_BUTTON_WAIT 5000

ButtonDebounce butOpen(BUT_OPEN_PIN, DEBOUNCE_TIME),
    butClose(BUT_CLOSE_PIN, DEBOUNCE_TIME);

State appState = PREBOOT;
uint32_t timestamp = 0, now = 0;

void setup() {
    timestamp = millis();
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
            if (butClose.state == HIGH) {
                if ((now - timestamp) >= BOOT_BUTTON_WAIT) {
                    appState = SETUP;
                }
            } else {
                appState = LOOP;
            }
            break;
        case SETUP:
            break;
    }
}