#pragma once
#include "sol/types.h"

typedef struct
{
    u32   maxHealth, maxEnergy, maxMana;
    u32   health, energy, mana;
    bool  doesRespawn;
    float deathTime, respawnTime;
    u32 team;
} VitalDesc;

void Sol_Vital_Init(World *world);
void Sol_Vital_Add(World *world, int id, VitalDesc desc);
void Sol_Vital_Step(World *world, double dt, double time);
void Sol_Vital_Damage(World *world, int id, SolHit hit);
u32  Sol_Vital_GetHealth(World *world, int id);
u32  Sol_Vital_GetMaxHealth(World *world, int id);
u32  Sol_Vital_GetTeam(World *world, int id);
u32  Sol_Vital_GetHostile(World *world, int id, int target);
bool Sol_Vital_GetIsalive(World *world, int id);