#include "config.h"

#include <ESP_EEPROM.h>

Config config;

const char *Config::AP_SSID = "ohm-led";
const char *Config::AP_PASSPHRASE = "password";

bool Config::Load()
{
    bool result = true;

    EEPROM.begin(sizeof(*this));
    EEPROM.get(0, *this);

    if (!isValid())
    {
        result = false;
        *this = Config();
    }

    EEPROM.end();

    if (num_leds == 0)
    {
        num_leds = DEFAULT_NUM_LEDS;
    }

    if (http_port == 0)
    {
        http_port = DEFAULT_HTTP_PORT;
    }

    if (fps <= 0)
    {
        fps = DEFAULT_FPS;
    }

    return result;
}

bool Config::Save() const
{
    // Make sure we write the magic number.
    magic_value = MAGIC_VALUE;

    EEPROM.begin(sizeof(*this));
    EEPROM.put(0, *this);
    EEPROM.commit();
    EEPROM.end();

    return true;
}

bool Config::Clear()
{
    magic_value = 0;

    EEPROM.begin(sizeof(*this));

    for (int i = 0; i < sizeof(*this); i++)
    {
        EEPROM.write(i, 0);
    }

    EEPROM.commit();
    EEPROM.end();

    return true;
}