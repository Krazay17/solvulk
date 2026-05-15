#pragma once
#include "sol/types.h"

#include "buff_types.h"
#include "combat/combat_types.h"

void Sol_Buff_Init(World *world);
void Sol_Buff_Clear(World *world, int id);

void Sol_Buff_Add(World *world, int id, BuffDesc desc, const SolHit *hit);
void Sol_Buff_Remove(World *world, int id, BuffKind kind);
void Sol_Buff_Step(World *world, double dt, double time);
bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);
