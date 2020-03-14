#pragma once

#include <cstdint>

#include <ArduinoJson.h>

#include "easing.h"

enum StateMode
{
    StateMode_Off = 0,
    StateMode_On = 1,
    StateMode_Pulse = 2,
    StateMode_Rainbow = 3,
    StateMode_Balls = 4,
    StateMode_KnightRider = 5,
    StateMode_Fire = 6,
    StateMode_Count,
};

enum StateUpdateResult
{
    StateUpdateResult_Success = 0,
    StateUpdateResult_InvalidInput = 1,
    StateUpdateResult_OutdatedInput = 2,
};

class State
{
public:
    StateUpdateResult fromJsonDocument(const StaticJsonDocument<256>& json);
    void toJsonDocument(StaticJsonDocument<256> &json);
    void cycle();
    void printState();
    int easeTime(Easing easing, int time, int mult);

    uint64_t revision = 0;
    StateMode mode = StateMode_Off;
    uint8_t hue = 0;
    uint8_t saturation = 0;
    uint8_t value = 255;
    Easing easing = EaseInOutQuad;
    uint32_t period = 5000;
    uint8_t num_balls = 1;
    uint8_t fire_cooling = 40;
    uint8_t fire_sparking = 80;
};

extern State state;

void setupState();
void stateLoop();
void cycleState();