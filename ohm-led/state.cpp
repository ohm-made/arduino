#include "state.h"

#include "config.h"
#include "easing.h"

#include <map>

#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#include <FastLED.h>

#define MIN_BALLS 1
#define MAX_BALLS 10

const std::map<StateMode, const char *> modeNames = {
    {StateMode_Off, "off"},
    {StateMode_On, "on"},
    {StateMode_Pulse, "pulse"},
    {StateMode_Colorloop, "colorloop"},
    {StateMode_Rainbow, "rainbow"},
    {StateMode_Balls, "balls"},
    {StateMode_KnightRider, "knight-rider"},
    {StateMode_Fire, "fire"},
};

const std::map<Easing, const char *> easingNames = {
    {EaseLinear, "linear"},
    {EaseInQuad, "in-quad"},
    {EaseOutQuad, "out-quad"},
    {EaseInOutQuad, "in-out-quad"},
    {EaseInCubic, "in-cubic"},
    {EaseOutCubic, "out-cubic"},
    {EaseInOutCubic, "in-out-cubic"},
    {EaseInElastic, "in-elastic"},
    {EaseOutElastic, "out-elastic"},
    {EaseInOutElastic, "in-out-elastic"},
    {EaseInBounce, "in-bounce"},
    {EaseOutBounce, "out-bounce"},
    {EaseInOutBounce, "in-out-bounce"},
    {EaseInExpo, "in-expo"},
    {EaseOutExpo, "out-expo"},
    {EaseInOutExpo, "in-out-expo"},
    {EaseInCirc, "in-circ"},
    {EaseOutCirc, "out-circ"},
    {EaseInOutCirc, "in-out-circ"},
};

StateMode modeFromString(const String &s)
{
    for (const auto &pair : modeNames)
    {
        if (s == pair.second)
        {
            return pair.first;
        }
    }

    return StateMode_Count;
}

String modeToString(StateMode mode)
{
    const auto it = modeNames.find(mode);

    if (it == modeNames.end())
    {
        // Let's default to off.
        return "off";
    }

    return it->second;
}

Easing easingFromString(const String &s)
{
    for (const auto &pair : easingNames)
    {
        if (s == pair.second)
        {
            return pair.first;
        }
    }

    return EaseCount;
}

String easingToString(Easing easing)
{
    const auto it = easingNames.find(easing);

    if (it == easingNames.end())
    {
        // Let's default to linear.
        return "linear";
    }

    return it->second;
}

StateUpdateResult State::fromJsonDocument(const StaticJsonDocument<256> &json)
{
    const StateMode newMode = modeFromString(json["mode"] | modeToString(mode));
    const Easing newEasing = easingFromString(json["easing"] | easingToString(easing));
    uint64_t requestRevision = json["revision"] | revision;

    if (newMode == StateMode_Count)
    {
        return StateUpdateResult_InvalidInput;
    }

    if (newEasing == EaseCount)
    {
        return StateUpdateResult_InvalidInput;
    }

    if (requestRevision != revision)
    {
        Serial.printf("Ignoring outdated state with revision %ul when %ul was expected.", requestRevision, revision);
        return StateUpdateResult_OutdatedInput;
    }

    mode = newMode;
    hue = json["hue"] | hue;
    saturation = json["saturation"] | saturation;
    value = json["value"] | value;
    easing = newEasing;
    period = json["period"] | period;
    num_balls = json["num-balls"] | num_balls;
    num_balls = min(uint8_t(MAX_BALLS), max(uint8_t(MIN_BALLS), num_balls));
    fire_cooling = json["fire-cooling"] | fire_cooling;
    fire_sparking = json["fire-sparking"] | fire_sparking;

    revision++;

    printState();

    return StateUpdateResult_Success;
}

void State::toJsonDocument(StaticJsonDocument<256> &json)
{
    json.clear();

    json["revision"] = revision;
    json["mode"] = modeToString(mode);
    json["hue"] = hue;
    json["saturation"] = saturation;
    json["value"] = value;
    json["easing"] = easingToString(easing);
    json["period"] = period;
    json["num-balls"] = num_balls;
    json["fire-cooling"] = fire_cooling;
    json["fire-sparking"] = fire_sparking;
}

void State::cycle()
{
    mode = static_cast<StateMode>(static_cast<int>(mode + 1));

    if (mode >= StateMode_Count)
    {
        mode = StateMode_Off;
    }

    revision++;

    printState();
}

void State::printState()
{
    Serial.printf("State: %s.\n", modeToString(mode).c_str());
    Serial.printf("HSV: %02x%02x%02x.\n", hue, saturation, value);
    Serial.printf("Period: %dms.\n", period);
    Serial.printf("Num balls: %d.\n", num_balls);
    Serial.printf("Fire cooling: %d.\n", fire_cooling);
    Serial.printf("Fire sparking: %d.\n", fire_sparking);
    Serial.printf("Easing: %s.\n", easingToString(easing).c_str());
}

State state;

CRGB leds[MAX_LEDS];

void setupState()
{
    FastLED.addLeds<WS2812, LEDS_DATA_PIN, GRB>(leds, config.num_leds);
    FastLED.setBrightness(255);
}

int State::easeTime(Easing easing, int time, int mult)
{
    if (period <= 0) {
        return 0;
    }

    const auto f = getEasingFunction(easing);
    int ts = time % (period * 2);

    if (ts >= period)
    {
        ts = 2 * period - ts;
    }

    const double rt = static_cast<double>(ts) / (period - 1);
    const double t = f(rt);

    return static_cast<int>(round(t * mult));
}

void pulse()
{
    const int fadeLevel = state.easeTime(state.easing, millis(), 255);

    fill_solid(leds, config.num_leds, CHSV(state.hue, state.saturation, state.value));
    fadeToBlackBy(leds, config.num_leds, fadeLevel);

    FastLED.show();
}

void colorloop()
{
    const int hue = state.easeTime(state.easing, millis(), 255);

    FastLED.showColor(CHSV(hue, state.saturation, state.value));
}

void rainbow()
{
    fill_rainbow(leds, config.num_leds, 0, 255 / config.num_leds);

    FastLED.show();
}

struct ballInfo {
    int timeOffset;
    double multiplier;
    CRGB color;
};

std::vector<ballInfo> initializeBalls()
{
    std::vector<ballInfo> ballsInfo;

    ballsInfo.resize(MAX_BALLS);

    for (int i = 0; i < MAX_BALLS; i++)
    {
        ballsInfo[i].timeOffset = random(state.period * 2);
        ballsInfo[i].multiplier = 1.0d + (random(50) / 50.0d);
        ballsInfo[i].color = CHSV((i * 255) / MAX_BALLS, state.saturation, state.value);
    }

    return ballsInfo;
}

void balls()
{
    static std::vector<ballInfo> ballsInfo = initializeBalls();

    fill_solid(leds, config.num_leds, CRGB::Black);

    for (int i = 0; i < state.num_balls; i++)
    {
        const int position = state.easeTime(state.easing, round(millis() * ballsInfo[i].multiplier) + ballsInfo[i].timeOffset, config.num_leds - 1);

        if (leds[position] == CRGB(CRGB::Black))
        {
            leds[position] = ballsInfo[i].color;
        }
        else
        {
            leds[position] = blend(ballsInfo[i].color, leds[position], 0.5);
        }
    }

    FastLED.show();
}

void knight_rider()
{
    const int position = state.easeTime(state.easing, millis(), config.num_leds - 1);

    for (int i = 0; i < config.num_leds; i++)
    {
        if (i == position)
        {
            leds[i] = CHSV(state.hue, state.saturation, state.value);
        }
        else
        {
            leds[i] = CRGB::Black;
        }
    }

    FastLED.show();
}

void fire()
{
    // Array of temperature readings at each simulation cell
    static byte heat[MAX_LEDS];

    // Step 1.  Cool down every cell a little
    for (int i = 0; i < config.num_leds; i++)
    {
        heat[i] = qsub8(heat[i], random8(0, ((state.fire_cooling * 10) / config.num_leds) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k = config.num_leds - 1; k >= 2; k--)
    {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < state.fire_sparking)
    {
        int y = random8(7);
        heat[y] = qadd8(heat[y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (int j = 0; j < config.num_leds; j++)
    {
        // Scale the heat value from 0-255 down to 0-240
        // for best results with color palettes.
        byte colorindex = scale8(heat[j], 240);
        CRGB color = ColorFromPalette(HeatColors_p, colorindex);

        leds[j] = color;
    }

    FastLED.show();
}

void stateLoop()
{
    {
        static int lastUpdate = millis();
        // Make sure we don't update fast than necessary.
        const int elapsed = millis() - lastUpdate;
        const int frame_time_left = (1000 / config.fps) - elapsed;

        if (frame_time_left > 0)
        {
            return;
        }

        lastUpdate = millis();
    }

    switch (state.mode)
    {
    case StateMode_Off:
        FastLED.showColor(CRGB::Black);
        break;
    case StateMode_On:
        FastLED.showColor(CHSV(state.hue, state.saturation, state.value));
        break;
    case StateMode_Pulse:
        pulse();
        break;
    case StateMode_Colorloop:
        colorloop();
        break;
    case StateMode_Rainbow:
        rainbow();
        break;
    case StateMode_Balls:
        balls();
        break;
    case StateMode_KnightRider:
        knight_rider();
        break;
    case StateMode_Fire:
        fire();
        break;
    default:
        FastLED.showColor(CRGB::Black);
        break;
    }
}