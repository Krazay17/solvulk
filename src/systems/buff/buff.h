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
    BUFFKIND_FIRE_MULT,
    BUFFKIND_STUN,
    BUFFKIND_SPEED,
    BUFFKIND_INVULN,
    BUFFKIND_INVULN_ADD,
    BUFFKIND_COUNT,
} BuffKind;

typedef void (*BuffEvent)(World *, int, int);
typedef struct
{
    u8        add, inf;
    float     duration, freq;
    BuffEvent onApply, onRemove;
} BuffConfig;

typedef struct
{
    u8    kind, inf;
    u32   source;
    float duration, accum;
    float freq, power;
} Buff;
typedef struct CompBuff
{
    Buff buffs[MAX_BUFFS];
    u32  count;
    u32  activeKindsMask;
} CompBuff;

void Sol_Buff_Init(World *world);

Buff *Sol_Buff_Add(World *world, int id, BuffKind kind, int source);
void  Sol_Buff_AddFromMask(World *world, int id, u32 mask, int sourceId);
void  Sol_Buff_Remove(World *world, int id, BuffKind kind);

void Sol_Buff_Step(World *world, double dt, double time);
bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);