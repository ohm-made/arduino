#include "reset.h"

#include "config.h"
#include "state.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>

void blinkWithPeriod(int duration, int durationMin, int durationMax, int period)
{
    if ((duration < durationMin) || (duration >= durationMax))
    {
        return;
    }

    digitalWrite(EXTERNAL_LED_PIN, (((duration / period) % 2) == 0) ? LOW : HIGH);
}

void resetLoop()
{
    static int pressedTime = millis();
    int pressedDuration = millis() - pressedTime;

    // If the button is not pressed, keep tracking the time of the last non-press to use as a reference.
    if (digitalRead(BUTTON_PIN) != LOW)
    {
        pressedTime = millis();
        digitalWrite(EXTERNAL_LED_PIN, LOW);

        if ((pressedDuration >= 50) && (pressedDuration < 1000))
        {
            state.cycle();
        }

        return;
    }

    blinkWithPeriod(pressedDuration, 0, 2000, 500);
    blinkWithPeriod(pressedDuration, 2000, 4000, 250);
    blinkWithPeriod(pressedDuration, 4000, 6000, 125);
    blinkWithPeriod(pressedDuration, 6000, 8000, 67);
    blinkWithPeriod(pressedDuration, 8000, 10000, 30);

    if (pressedDuration >= 10000)
    {
        config.Clear();
        ESP.restart();
    }
}