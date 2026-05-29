#pragma once
#include "sol/types.h"

typedef enum
{
    VITALKIND_PLAYER,
    VITALKIND_WIZARD,
} VitalKind;

typedef struct CompVital
{
    u32    maxHealth, maxEnergy, maxMana;
    u32    health, energy, mana;
    bool   doesRespawn;
    float  respawnTime;
    double deathTime, lastHitTime;
} CompVital;

void Sol_Vital_Init(World *world);
void Sol_Vital_Add(World *world, int id, VitalKind kind);

void  Sol_Vital_Step(World *world, double dt, double time);
void  Sol_Vital_Damage(World *world, int id, const SolHit *hit);
u32   Sol_Vital_GetHealth(World *world, int id);
u32   Sol_Vital_GetMaxHealth(World *world, int id);
bool  Sol_Vital_GetDead(World *world, int id);
float Sol_Vital_GetLastHitTime(World *world, int id);
void  Sol_Vital_Die(World *world, int id);
void  Sol_Vital_Respawn(World *world, int id);