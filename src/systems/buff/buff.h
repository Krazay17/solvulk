#pragma once
#include "sol/types.h"

#include "combat/combat_types.h"

#define MAX_BUFFS 64
#define BASE_TICK_INTERVAL 1.0f

#define BuffBit(a) (1 << a)

typedef enum
{
    BUFFADDKIND_SET_DURATION,
    BUFFADDKIND_ADD_DURATION,
    BUFFADDKIND_MULTIPLY,
    BUFFADDKIND_INF,
} BuffAddKind;

typedef enum
{
    BUFFKIND_KNOCKBACK,
    BUFFKIND_FIRE,
    BUFFKIND_SPEED,
    BUFFKIND_INVULN,
    BUFFKIND_COUNT,
} BuffKind;

typedef struct
{
    u8    kind, addKind;
    u32   inf, source;
    vec3s dir, pos;
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
void Sol_Buff_Clear(World *world, int id);

void Sol_Buff_Add(World *world, int id, BuffKind kind, const SolHit *hit);
void Sol_Buff_Remove(World *world, int id, BuffKind kind);
void Sol_Buff_Step(World *world, double dt, double time);
bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);
