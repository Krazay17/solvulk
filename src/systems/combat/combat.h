#pragma once
#include "sol/types.h"

#include "combat_types.h"

#define CHAIN_SIZE 0xff

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

typedef struct
{
    int   count, dealer, last;
    int   hitEnts[MAX_ENTS];
    float accum, delay;

} Chain;
typedef struct CompChainhit
{
    Chain *chains;
    u32    count;
    u32    capacity;
} CompChainhit;

void Sol_Combat_Init(World *world);
bool Sol_Combat_IsReflecting(World *world, int id);
void Sol_Combat_AddFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_RemoveFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_ClearFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_ClearHits(World *world, int id);

void Chain_Lightning_Recursive(World *world, int dealer, int target, int last, u32 damage, int count);
void Sol_Chainhit_Init(World *world);
void Sol_Chainhit_Trigger(World *world, int dealer, int target, u32 count);