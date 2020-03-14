#pragma once

#include <cstdint>
#include <cstring>

// The maximum support number of leds.
#define MAX_LEDS 128

// PIN configuration.
#define LEDS_DATA_PIN 1
#define EXTERNAL_LED_PIN D3
#define BUTTON_PIN D5

class Config
{
public:
    static const char* AP_SSID;
    static const char* AP_PASSPHRASE;
    static const uint64_t MAGIC_VALUE = 0xBADA551;
    static const uint16_t DEFAULT_HTTP_PORT = 80;
    static const uint16_t DEFAULT_NUM_LEDS = 16;
    static const int DEFAULT_FPS = 120;

    Config() = default;
    bool Load();
    bool Save() const;
    bool Clear();

private:
    mutable uint64_t magic_value = 0;

    Config(const Config &) = default;
    Config &operator=(const Config &) = default;

    bool isValid() const
    {
        return magic_value == MAGIC_VALUE;
    }

public:
    char name[32] = "ohm-led";
    char ssid[64] = {};
    char passphrase[64] = {};
    uint16_t http_port = DEFAULT_HTTP_PORT;
    uint16_t num_leds = DEFAULT_NUM_LEDS;
    int fps = DEFAULT_FPS;

    bool hasName() const
    {
        return (strnlen(name, sizeof(name)) > 0);
    }

    bool hasSSID() const
    {
        return (strnlen(ssid, sizeof(ssid)) > 0);
    }
};

extern Config config;