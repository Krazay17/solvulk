#pragma once
#include "sol/types.h"

typedef enum
{
    COMBATFLAG_REFLECTING = (1 << 0),
} CombatFlags;

typedef struct CompCombat
{
    CombatFlags flags;
    u32         damage, hitPauseDiminish;
    bool        hitEnts[MAX_ENTS];
    float       hitPause, baseAnimRate;
} CompCombat;

typedef enum
{
    CHAINKIND_LIGHTNING,
} ChainKind;
typedef struct
{
    u8    kind;
    float damage;
    int   count, dealer, last;
    int   hitEnts[MAX_ENTS];
    float accum, delay;
} Chain;
typedef struct ChainAttacks
{
    Chain *chains;
    u32    count;
    u32    capacity;
} ChainAttacks;

void Sol_Combat_Init(World *world);
bool Sol_Combat_IsReflecting(World *world, int id);
void Sol_Combat_AddFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_RemoveFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_ClearFlags(World *world, int id, CombatFlags flags);
void Sol_Combat_ClearHits(World *world, int id);

void Chain_Lightning_Recursive(World *world, int dealer, int target, int last, float damage, int count);
void Sol_Chainhit_Init(World *world);
void Sol_Chainhit_Trigger(World *world, int dealer, int target, ChainKind kind, float damage);