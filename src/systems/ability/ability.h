#pragma once
#include "sol/types.h"

typedef enum
{
    ABILITY_STATE_IDLE,
    ABILITY_STATE_DASH,
    ABILITY_STATE_CLAW,
    ABILITY_STATE_FIREBALL,
    ABILITY_STATE_SHIELD,
    ABILITY_STATE_FIREBALLVOLLEY,
    ABILITY_STATE_COUNT,
} AbilityState;

typedef enum
{
    ABILITYSPAWN_FIREBALL,
} AbilitySpawnId;

typedef struct
{
    SolActions   actionBit;
    AbilityState targetState;
} AbilityMapping;

typedef struct
{
    u32            hasAbilies;
    AbilityMapping abilityMapping[ABILITY_STATE_COUNT];
} AbilityDesc;

void Sol_Ability_Init(World *world);
void Sol_Ability_Add(World *world, int id, AbilityDesc desc);
void Sol_Ability_Step(World *world, double dt, double time);
void Sol_Ability_Tick(World *world, double dt, double time);

bool         Sol_Ability_SetState(World *world, int id, AbilityState state);
AbilityState Sol_Ability_GetState(World *world, int id);
