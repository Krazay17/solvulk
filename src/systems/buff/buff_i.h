#pragma once
#include "sol/types.h"

#include "buff.h"

#define MAX_BUFFS 64
#define BASE_TICK_INTERVAL 1.0f

typedef struct
{
    BuffKind kind;
    float    duration, accum, freq;
    u32      inf;
} Buff;
typedef struct CompBuff
{
    Buff   buffs[MAX_BUFFS];
    u32    count;
    SolHit lasthit;
} CompBuff;
