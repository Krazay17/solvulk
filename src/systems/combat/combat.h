#pragma once
#include "sol/types.h"

#include "combat_types.h"

typedef enum
{
    COMBATFLAG_REFLECTING = (1 << 0),
} CombatFlags;

typedef struct
{
    int   count;
    int   hitEnts[MAX_ENTS];
    int last;
    float accum, delay;
} ChainLightning;

typedef struct CompCombat
{
    CombatFlags    flags;
    u32            damage;
    bool           hitEnts[MAX_ENTS];
    ChainLightning chainLightning;
} CompCombat;

void Sol_Combat_Init(World *world);
bool Sol_Combat_IsReflecting(World *world, int id);
void Sol_Combat_AddFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_RemoveFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_ClearFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_ClearHits(World *world, int id);

void Chain_Lightning(World *world, int dealer, int target, int last, u32 damage, int count);
void Chain_Lightning_NoReassess(World *world, int dealer, int target, u32 damage, int count);
void Chain_Lightning_Single(World *world, int dealer, int last, u32 count);