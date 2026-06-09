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
    vec3s        enterDir;
    bool         held;
    float        elapsed, duration, accum, recovery, charge, cooldown;
    double       lastEntered, lastExited;
    u32          stage;
    u32          damage;
    u32          buffs;
    u32          effects;
} AbilityData;
typedef struct
{
    SolActions actionBit;

    u32 boundState;
    u32 boundRarity;
    u32 boundBonusDamage;
    u32 boundBonusBuffs;
    u32 boundBonusEffects;

    u32 pendingState;
    u32 pendingRarity;
    u32 pendingBonusDamage;
    u32 pendingBonusBuffs;
    u32 pendingBonusEffects;

    bool dirtyApply, dirtySend;
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

typedef enum
{
    EFFECTMASK_KNOCKBACK        = (1 << 0),
    EFFECTMASK_KNOCKBACK_STRONG = (1 << 1),
    EFFECTMASK_KNOCKUP          = (1 << 2),
} EffectMask;
typedef struct
{
    char  name[32];
    float cooldown, duration;
    u32   damage;
    u8    buffMask;
    u8    effectMask;
} AbilityConfig;

extern AbilityConfig ability_config[ABILITY_STATE_COUNT][3];

void Sol_Ability_Init(World *world);
void Sol_Ability_Add(World *world, int id, AbilityDesc desc);
void Sol_Ability_Step(World *world, double dt, double time);

bool         Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force);
AbilityState Sol_Ability_GetState(World *world, int id);
const char  *Sol_Ability_GetNameString(u32 ability);

void Sol_Ability_RequestBind(World *world, int id, u32 slot, u32 ability, u32 rarity, u32 bonusDamage, u32 bonusBuffs);
void Sol_Ability_Bind(World *world, int id, u32 slot, u32 ability, u32 rarity, u32 bonusDamage, u32 bonusBuffs);