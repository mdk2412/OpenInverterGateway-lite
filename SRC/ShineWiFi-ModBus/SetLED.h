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
    void set(LedColor led, LedMode mode, uint32_t blinkMs);

    void on(LedColor led);
    void off(LedColor led);
    void blink(LedColor led, uint32_t interval);

    /**
     * Steuert die Status-LEDs basierend auf den aktuellen Systemzuständen:
     * - WiFi + Modbus + MQTT  -> Grün blinkt
     * - WiFi + Modbus         -> Blau blinkt
     * - Nur Modbus (kein WiFi)-> Rot blinkt
     * - Sonst                 -> Alle aus
     */
    void updateStatus(bool wifiOK, bool modbusOK, bool mqttOK);

    void loop();

private:
    struct LedState {
        uint8_t  pin;
        LedMode  mode;
        bool     state;
        uint32_t interval;
        uint32_t lastToggle;
        bool     enabled;
        bool     activeLevel;   // HIGH = AN, LOW = AN
    };

    LedState leds[3];

    // zentrale LED-Schreibfunktion
    void writeLed(LedState &l, bool on);
};

extern SetLEDClass SetLED;

#endif