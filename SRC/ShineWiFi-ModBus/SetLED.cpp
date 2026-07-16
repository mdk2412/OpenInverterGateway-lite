#include "SetLED.h"
#include "Config.h"

SetLEDClass SetLED;

// zentrale LED-Schreibfunktion
void SetLEDClass::writeLed(LedState &l, bool on)
{
    digitalWrite(l.pin, (on == l.activeHigh) ? HIGH : LOW);
}

void SetLEDClass::begin()
{
    // 255 = LED nicht vorhanden
    leds[LED_RED] = {
        LED_RD,
        LED_OFF,
        false,              // state
        0,                  // interval
        0,                  // lastToggle (WICHTIG!)
        false,              // enabled
        LED_RD_ACTIVE_HIGH  // Polarität aus config.h
    };

    leds[LED_GREEN] = {
        LED_GN,
        LED_OFF,
        false,
        0,
        0,
        false,
        LED_GN_ACTIVE_HIGH
    };

    leds[LED_BLUE] = {
        LED_BL,
        LED_OFF,
        false,
        0,
        0,
        false,
        LED_BL_ACTIVE_HIGH
    };

    for (uint8_t i = 0; i < 3; i++)
    {
        if (leds[i].pin != 255)
        {
            pinMode(leds[i].pin, OUTPUT);
            writeLed(leds[i], false);   // LED AUS
        }
    }
}

void SetLEDClass::set(LedColor led, LedMode mode, uint32_t blinkMs)
{
    LedState &l = leds[led];

    if (l.pin == 255)
        return;

    if (l.mode == mode && l.interval == blinkMs)
        return;

    l.mode = mode;

    switch (mode)
    {
        case LED_OFF:
            l.enabled = false;
            l.state = false;
            writeLed(l, false);
            break;

        case LED_ON:
            l.enabled = true;
            l.interval = 0;
            l.state = true;
            writeLed(l, true);
            break;

        case LED_BLINK:
            l.enabled = true;
            l.interval = blinkMs;
            l.lastToggle = millis();
            l.state = true;
            writeLed(l, true);
            break;
    }
}

void SetLEDClass::on(LedColor led)
{
    set(led, LED_ON, 0);
}

void SetLEDClass::off(LedColor led)
{
    set(led, LED_OFF, 0);
}

void SetLEDClass::blink(LedColor led, uint32_t interval)
{
    set(led, LED_BLINK, interval);
}

void SetLEDClass::loop()
{
    uint32_t now = millis();

    for (uint8_t i = 0; i < 3; i++)
    {
        LedState &l = leds[i];

        if (l.pin == 255)
            continue;

        if (!l.enabled)
            continue;

        if (l.mode != LED_BLINK)
            continue;

        if (now - l.lastToggle >= l.interval)
        {
            l.lastToggle = now;
            l.state = !l.state;
            writeLed(l, l.state);
        }
    }
}
