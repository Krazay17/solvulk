#pragma once
#include "sol/types.h"

typedef struct SolHit
{
    vec3s pos, dir;
    float power;
    u32   damage;
} SolHit;
typedef struct
{
    u32   maxHealth, maxEnergy, maxMana;
    u32   health, energy, mana;
    bool  doesRespawn;
    float deathTime, respawnTime;
} VitalDesc;
void Sol_Vital_Init(World *world);
void Sol_Vital_Add(World *world, int id, VitalDesc desc);
void Sol_Vital_Draw(World *world, double dt, double time);
void Sol_Vital_Step(World *world, double dt, double time);
