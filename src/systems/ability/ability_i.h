#pragma once
#include "sol/types.h"

#include "ability.h"
#include "estate.h"

typedef struct
{
    vec3s dir, pos;
    float lastEntered, elapsed, duration, accum;
} AbilityData;
typedef struct CompAbility
{
    vec3s        attackPos, attackDir;
    AbilityState state;
    AbilityData  stateData[ABILITY_STATE_COUNT];
} CompAbility;

typedef struct
{
    SolActions   actionBit;
    AbilityState targetState;
} AbilityMapping;

extern const StateFunc ability_state_func[];

bool Sol_Ability_SetState(World *world, int id, AbilityState state);

void IdleAbility_State_Update(World *world, int id, float dt);
void IdleAbility_State_Enter(World *world, int id);
void IdleAbility_State_Exit(World *world, int id);
bool IdleAbility_State_CanExit(World *world, int id, int next);
bool IdleAbility_State_CanEnter(World *world, int id, int last);

void Claw_State_Update(World *world, int id, float dt);
void Claw_State_Enter(World *world, int id);
void Claw_State_Exit(World *world, int id);
bool Claw_State_CanExit(World *world, int id, int next);
bool Claw_State_CanEnter(World *world, int id, int last);

void ADash_State_Update(World *world, int id, float dt);
void ADash_State_Enter(World *world, int id);
void ADash_State_Exit(World *world, int id);
bool ADash_State_CanExit(World *world, int id, int next);
bool ADash_State_CanEnter(World *world, int id, int last);

void Shield_State_Update(World *world, int id, float dt);
void Shield_State_Enter(World *world, int id);
void Shield_State_Exit(World *world, int id);
bool Shield_State_CanExit(World *world, int id, int next);
bool Shield_State_CanEnter(World *world, int id, int last);
