#pragma once
#include "base.h"

typedef enum ZoneKind
{
    ZONEKIND_FIRE,
    ZONEKIND_DAMAGE,
    ZONEKIND_HEAL,
    ZONEKIND_COUNT,
} ZoneKind;

typedef struct CompZone
{
    u32   kind;
    float duration, rate, value, radius;
    float accum;
} CompZone;

void Sol_Zone_Init(World *world);
void Sol_Zone_Add(World *world, int id, ZoneKind kind);