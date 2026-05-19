#pragma once

#include <stdint.h>

#define MAX_CLIENTS 10

#pragma pack(push, 1) // Tells the compiler: "No padding, keep it tight"
typedef struct
{
    uint8_t id;
    float x, y, z;
    uint8_t inputState;
    uint8_t active;
} PlayerPacket;
#pragma pack(pop) // Restore normal alignment for the rest of your code

typedef struct
{
    PlayerPacket players[MAX_CLIENTS];
} WorldState;