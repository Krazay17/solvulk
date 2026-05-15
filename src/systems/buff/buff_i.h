#pragma once
#include "sol/types.h"

#include "buff.h"

#define MAX_BUFFS 64
#define BASE_TICK_INTERVAL 1.0f

typedef struct
{
    vec3s    dir, pos;
    BuffKind kind;
    float    duration, accum, initialDuration;
    float    freq, power;
    u32      inf, source;
} Buff;
typedef struct CompBuff
{
    Buff buffs[MAX_BUFFS];
    u32  count;
} CompBuff;
