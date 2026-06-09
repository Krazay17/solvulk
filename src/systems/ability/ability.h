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
    u32          bonusDamage;
    u8           buffs;
} AbilityData;
typedef struct
{
    SolActions actionBit;
    u32        boundState;
    u32        boundRarity;
    u32        boundBonusDamage;
    u8         boundBonusBuffs;
    u32        pendingState;
    u32        pendingRarity;
    u32        pendingBonusDamage;
    u8         pendingBonusBuffs;
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
    char  name[32];
    u32   damage;
    float cooldown, duration;
    u8    buffMask;
} AbilityConfig;

extern AbilityConfig ability_config[ABILITY_STATE_COUNT][3];

void Sol_Ability_Init(World *world);
void Sol_Ability_Add(World *world, int id, AbilityDesc desc);
void Sol_Ability_Step(World *world, double dt, double time);

bool         Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force);
AbilityState Sol_Ability_GetState(World *world, int id);
const char  *Sol_Ability_GetNameString(u32 ability);

void Sol_Ability_RequestBind(World *world, int id, u32 slot, u32 ability, u32 rarity, u32 bonusDamage, u8 bonusBuffs);
void Sol_Ability_Bind(World *world, int id, u32 slot, u32 ability, u32 rarity, u32 bonusDamage, u8 bonusBuffs);