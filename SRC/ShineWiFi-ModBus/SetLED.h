#ifndef _SETLED_H_
#define _SETLED_H_

#include <Arduino.h>

enum LedColor : uint8_t {
    LED_RED = 0,
    LED_GREEN,
    LED_BLUE
};

enum LedMode : uint8_t {
    LED_OFF = 0,
    LED_ON,
    LED_BLINK
};

class SetLEDClass {
public:
    void begin();

    // neue API:
    // LED_OFF   = LED aus
    // LED_ON    = Dauerlicht
    // LED_BLINK = Blinkintervall in ms
    void set(LedColor led, LedMode mode, uint32_t blinkMs);

    void on(LedColor led);
    void off(LedColor led);
    void blink(LedColor led, uint32_t interval);

    void loop();

private:
    struct LedState {
        uint8_t  pin;
        LedMode  mode;
        bool     state;
        uint32_t interval;
        uint32_t lastToggle;
        bool     enabled;
        bool     activeLevel;   // NEU: HIGH = AN, LOW = AN
    };

    LedState leds[3];

    // NEU: zentrale LED‑Schreibfunktion
    void writeLed(LedState &l, bool on);
};

extern SetLEDClass SetLED;

#endif
