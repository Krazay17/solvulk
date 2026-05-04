#pragma once
#include "sol/types.h"
#include "estate.h"

#include "controller/controller.h"
#include "xform/xform.h"

typedef struct
{
    float lastEntered, elapsed, duration;
} AbilityData;
typedef struct CompAbility
{
    vec3s        attackPos, attackDir;
    AbilityState state;
    AbilityData  stateData[ABILITY_STATE_COUNT];
} CompAbility;

typedef struct
{
    PlayerActionStates actionBit;
    AbilityState       targetState;
} AbilityMapping;

extern const StateFunc ability_state_func[];

bool Sol_Ability_SetState(World *world, int id, AbilityState state);

void IdleAbility_State_Update(World *world, int id, float dt);
void IdleAbility_State_Enter(World *world, int id);
void IdleAbility_State_Exit(World *world, int id);
bool IdleAbility_State_CanEnter(World *world, int id);
bool IdleAbility_State_CanExit(World *world, int id);

void Claw_State_Update(World *world, int id, float dt);
void Claw_State_Enter(World *world, int id);
void Claw_State_Exit(World *world, int id);
bool Claw_State_CanEnter(World *world, int id);
bool Claw_State_CanExit(World *world, int id);
