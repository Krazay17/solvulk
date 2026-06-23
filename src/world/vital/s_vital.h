#pragma once
#include "base.h"

typedef struct World World;

typedef struct CompVital
{
    float  maxHealth, maxEnergy, maxMana;
    float  health, energy, mana;
    bool   doesRespawn;
    float  respawnTime;
    double deathTime, lastHitTime;
} CompVital;

typedef enum
{
    VITALKIND_PLAYER,
    VITALKIND_WIZARD,
} VitalKind;
void Sol_Vital_Init(World *world);
void Sol_Vital_Add(World *world, int id, VitalKind kind);

float Sol_Vital_GetHealth(World *world, int id);
float Sol_Vital_GetMaxHealth(World *world, int id);
bool  Sol_Vital_GetDead(World *world, int id);
float Sol_Vital_GetLastHitTime(World *world, int id);
bool  Sol_Vital_Damage(World *world, int id, int attacker, float damage);
void  Sol_Vital_Heal(World *world, int id, int healer, u32 heal);
