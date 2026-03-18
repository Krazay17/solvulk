#pragma once
#include <stdint.h>


typedef enum
{
    ACTION_NONE     = 0,
    ACTION_FWD      = 1 << 0,
    ACTION_BWD      = 1 << 1,
    ACTION_LEFT     = 1 << 2,
    ACTION_RIGHT    = 1 << 3,
    ACTION_JUMP     = 1 << 4,
    ACTION_COUNT    = 1 << 5,
} PlayerActionStates;

typedef struct
{
    float yaw, pitch;
    uint32_t actionState;
} PlayerState;

PlayerState *Sol_GetPlayerState();