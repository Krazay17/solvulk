#pragma once
#include "base.h"

#define MAX_BUFFS 64

typedef struct World World;

typedef enum
{
    BUFFKIND_FIRE,
    BUFFKIND_STUN,
    BUFFKIND_SPEED,
    BUFFKIND_INVULN,
    BUFFKIND_COUNT,
} BuffKind;

typedef enum
{
    BUFFADD_SET,
    BUFFADD_ADD,
    BUFFADD_MULTIPLY,
} BuffAdd;

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

void Sol_Buff_Add(World *world, int id, int source, BuffKind kind);
void Sol_Buff_AddEx(World *world, int id, int source, BuffKind kind, float duration, float power);
void Sol_Buff_AddFromMask(World *world, int id, int source, u32 mask);
void Sol_Buff_Remove(World *world, int id, BuffKind kind);

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);
u32 Sol_Buff_GetMask(World *world, int id);