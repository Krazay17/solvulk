#pragma once
#include "base.h"

typedef struct World World;

typedef struct CompCombat
{
    u16   flags;
    u32   damage, hitPauseDiminish;
    bool  hitEnts[65536];
    float hitPause, baseAnimRate;
} CompCombat;

typedef struct Dmgnumber
{
    int   amnt;
    float ttl;
    vec3s pos;
    vec4s color;
} Dmgnumber;
typedef struct Dmgnumbers
{
    Dmgnumber *dmgNumber;
    int        count;
    int        cap;
} Dmgnumbers;

typedef enum
{
    CHAINKIND_LIGHTNING,
} ChainKind;
typedef struct
{
    u8    kind;
    float damage;
    int   count, dealer, last;
    int   hitEnts[65536];
    float accum, delay;
} Chain;
typedef struct ChainAttacks
{
    Chain *chains;
    u32    count;
    u32    capacity;
} ChainAttacks;

void Sol_Combat_Init(World *world);
void Sol_Chainhit_Init(World *world);

void Sol_Combat_Add(World *world, int id);
bool Sol_Combat_IsReflecting(World *world, int id);
void Sol_Combat_AddFlags(World *world, int id, u32 flags);
void Sol_Combat_RemoveFlags(World *world, int id, u32 flags);
void Sol_Combat_ClearFlags(World *world, int id, u32 flags);
void Sol_Combat_ClearHits(World *world, int id);

// void Chain_Lightning_Recursive(World *world, int dealer, int target, int last, float damage, int count);
void Sol_Chainhit_Trigger(World *world, int dealer, int target, u32 kind, float damage);

void Sol_Dmgnumbers_Spawn(World *world, int id, int amnt, vec3s pos);

void Sol_Weapon_Equip(World *world, int id, int weapon, int slot);

void Dmgnumbers_Draw(World *world, double dt, double time);
void Dmgnumbers_Step(World *world, double dt, double time);
void Weapon_Step(World *world, double dt, double time);