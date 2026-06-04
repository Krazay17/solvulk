#pragma once
#include "sol/types.h"

#include "combat_types.h"

typedef enum
{
    COMBATFLAG_REFLECTING = (1 << 0),
} CombatFlags;

typedef struct CompCombat
{
    CombatFlags flags;
    u32         damage;
    bool        hitEnts[MAX_ENTS];
} CompCombat;

void Sol_Combat_Init(World *world);
bool Sol_Combat_IsReflecting(World *world, int id);
void Sol_Combat_AddFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_RemoveFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_ClearFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_ClearHits(World *world, int id);