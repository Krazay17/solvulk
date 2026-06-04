#pragma once
#include "sol/types.h"

#define MAX_MAPPED_SKILLS 10

typedef enum
{
    ABILITYSPAWN_FIREBALL,
} AbilitySpawnId;

typedef struct
{
    vec3s  dir, pos;
    bool   held;
    float  elapsed, duration, accum, recovery, charge;
    double lastEntered, lastExited;
    u32    stage;
    u32    hitEnts[256];
} AbilityData;
typedef struct
{
    SolActions   actionBit;
    AbilityState targetState;
} SkillBinding;
typedef struct CompAbility
{
    AbilityState state;
    AbilityData  stateData[MAX_MAPPED_SKILLS];
    SkillBinding bindings[MAX_MAPPED_SKILLS];
    u32          activeSlot;
} CompAbility;

typedef struct
{
    SkillBinding bindings[MAX_MAPPED_SKILLS];
} AbilityDesc;

void Sol_Ability_Init(World *world);
void Sol_Ability_Add(World *world, int id, AbilityDesc desc);
void Sol_Ability_Step(World *world, double dt, double time);

bool         Sol_Ability_SetState(World *world, int id, AbilityState nextState, u32 slot, bool force);
AbilityState Sol_Ability_GetState(World *world, int id);
void         Sol_Ability_SetAbility(World *world, int id, u32 slotIndex, AbilityState ability);