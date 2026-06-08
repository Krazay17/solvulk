#pragma once
#include "sol/types.h"

#define MAX_MAPPED_SKILLS 10

typedef struct CompItem CompItem;

typedef enum
{
    ABILITY_STATE_IDLE,
    ABILITY_STATE_DASH,
    ABILITY_STATE_FIREBALL,
    ABILITY_STATE_PISTOL,
    ABILITY_STATE_SPINSLASH,
    ABILITY_STATE_CLAW,
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
    AbilityState kind;
    vec3s        dir, pos;
    bool         held;
    float        elapsed, duration, accum, recovery, charge;
    float        cooldown, damage;
    double       lastEntered, lastExited;
    u32          stage;
    u32          hitEnts[256];
} AbilityData;
typedef struct
{
    SolActions actionBit;
    u32        boundState;
    u32        boundRarity;
    u32        pendingState;
    u32        pendingRarity;
    bool       dirtyApply, dirtySend;
} SkillBinding;

typedef struct CompAbility
{
    int          activeSlot;
    AbilityState state;
    AbilityData  stateData[MAX_MAPPED_SKILLS];
    SkillBinding bindings[MAX_MAPPED_SKILLS];
} CompAbility;

typedef struct
{
    SkillBinding bindings[MAX_MAPPED_SKILLS];
} AbilityDesc;

typedef struct
{
    u32   damage;
    float cooldown, duration, fireRate;
} AbilityConfig;

extern AbilityConfig ability_config[ABILITY_STATE_COUNT][3];

void Sol_Ability_Init(World *world);
void Sol_Ability_Add(World *world, int id, AbilityDesc desc);
void Sol_Ability_Step(World *world, double dt, double time);

bool         Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force);
AbilityState Sol_Ability_GetState(World *world, int id);
void         Sol_Ability_RequestBind(World *world, int id, u32 slot, u32 ability, u32 rarity);
void         Sol_Ability_Bind(World *world, int id, u32 slot, u32 ability, u32 rarity);