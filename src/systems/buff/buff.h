#pragma once
#include "sol/types.h"

#include "combat/combat_types.h"

#define MAX_BUFFS 64
#define BASE_TICK_INTERVAL 1.0f

typedef enum
{
    BUFFADD_SET,
    BUFFADD_ADD,
    BUFFADD_MULTIPLY,
} BuffAdd;

typedef enum
{
    BUFFKIND_FIRE,
    BUFFKIND_CHAIN_LIGHTNING,
    BUFFKIND_FIRE_MULT,
    BUFFKIND_STUN,
    BUFFKIND_SPEED,
    BUFFKIND_INVULN,
    BUFFKIND_INVULN_ADD,
    BUFFKIND_COUNT,
} BuffKind;

typedef struct
{
    u8    kind, add;
    u32   inf, source, bounces;
    vec3s vel;
    float duration, accum, initialDuration;
    float freq, power;
} Buff;
typedef struct CompBuff
{
    Buff buffs[MAX_BUFFS];
    u32  count;
    u32  activeKindsMask;
} CompBuff;

void Sol_Buff_Init(World *world);

Buff *Sol_Buff_Add(World *world, int id, BuffKind kind);
void  Sol_Buff_AddFromMask(World *world, int id, u32 mask, int sourceId);

void Sol_Buff_Remove(World *world, int id, BuffKind kind);
void Sol_Buff_Step(World *world, double dt, double time);
bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);