#pragma once
#include "sol/types.h"

typedef enum
{
    ABILITY_STATE_IDLE,
    ABILITY_STATE_DASH,
    ABILITY_STATE_CLAW,
    ABILITY_STATE_SHIELD,
    ABILITY_STATE_3,
    ABILITY_STATE_4,
    ABILITY_STATE_5,
    ABILITY_STATE_6,
    ABILITY_STATE_7,
    ABILITY_STATE_8,
    ABILITY_STATE_9,
    ABILITY_STATE_COUNT,
} AbilityState;

typedef struct
{
    u32 hasAbilies;
} AbilityDesc;

void Sol_Ability_Init(World *world);
void Sol_Ability_Add(World *world, int id, AbilityDesc desc);
void Sol_Ability_Step(World *world, double dt, double time);
void Sol_Ability_Tick(World *world, double dt, double time);

AbilityState Sol_Ability_GetState(World *world, int id);
