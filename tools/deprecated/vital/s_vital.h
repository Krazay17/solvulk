#pragma once
#include "base.h"

typedef struct World World;

typedef struct CompCombat
{
    vec3s  respawnPos;
    float  maxHealth, maxEnergy, maxMana;
    float  health, energy, mana;
    bool   doesRespawn;
    float  respawnTime;
    double deathTime, lastHitTime;
} CompCombat;

typedef enum
{
    COMBATKIND_PLAYER,
    COMBATKIND_WIZARD,
} VitalKind;
void Sol_Vital_Init(World *world);
void Sol_Combat_Add(World *world, int id, VitalKind kind);

float Sol_Combat_GetHealth(World *world, int id);
bool  Sol_Combat_GetDead(World *world, int id);
float Sol_Vital_Damage(World *world, int id, int attacker, float damage);
float Sol_Vital_Heal(World *world, int id, int dealer, float amount);
