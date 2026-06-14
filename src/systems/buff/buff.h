#pragma once
#include "sol/types.h"

#include "combat/combat_types.h"

typedef enum
{
    BUFFKIND_FIRE,
    BUFFKIND_STUN,
    BUFFKIND_SPEED,
    BUFFKIND_INVULN,
    BUFFKIND_COUNT,
} BuffKind;

void Sol_Buff_Init(World *world);

void Sol_Buff_Add(World *world, int id, int source, BuffKind kind);
void Sol_Buff_AddEx(World *world, int id, int source, BuffKind kind, float duration, float power);
void Sol_Buff_AddFromMask(World *world, int id, int source, u32 mask);
void Sol_Buff_Remove(World *world, int id, BuffKind kind);

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);
u32 Sol_Buff_GetMask(World *world, int id);