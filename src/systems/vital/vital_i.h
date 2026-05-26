#pragma once
#include "sol/types.h"
#include "combat/combat_types.h"

typedef struct CompVital
{
    u32   maxHealth, maxEnergy, maxMana;
    u32   health, energy, mana;
    bool  doesRespawn;
    float deathTime, respawnTime, lastHitTime;
    u32   team;
} CompVital;

void Respawn(World *world, int id, CompVital *vital);
void Die(World *world, int id, SolHit hit);
