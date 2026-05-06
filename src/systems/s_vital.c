#include "sol_core.h"
#include "xform/xform.h"

typedef struct CompVital
{
    u32   maxHealth, maxEnergy, maxMana;
    u32   health, energy, mana;
    bool  doesRespawn;
    float deathTime, respawnTime;
} CompVital;

void Respawn(World *world, int id, CompVital *vital);
void Die(World *world, int id, CompVital *vital);
void Damage(World *world, int id, CompVital *vital, SolHit hit);

void Sol_Vital_Init(World *world)
{
    world->vitals = calloc(MAX_ENTS, sizeof(CompVital));
}

void Sol_Vital_Add(World *world, int id, VitalDesc desc)
{
    world->masks[id] |= HAS_VITAL;
    CompVital vital = {
        .maxHealth = desc.maxHealth,
        .maxEnergy = desc.maxEnergy,
        .maxMana   = desc.maxMana,
        .health    = desc.maxHealth,
        .energy    = desc.maxEnergy,
        .mana      = desc.maxMana,
    };
    world->vitals[id] = vital;
}

void Sol_Vital_Remove(World *world, int id)
{
    memset(&world->vitals[id], 0, sizeof(CompVital));
    world->masks[id] &= ~HAS_VITAL;
}

void Sol_Vital_Step(World *world, double dt, double time)
{
    int required = HAS_VITAL;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompVital *vital = &world->vitals[id];
        if (vital->doesRespawn && (float)time > vital->deathTime + vital->respawnTime)
            Respawn(world, id, vital);
    }
}

void Sol_Vital_Draw(World *world, double dt, double time)
{
    int required = HAS_VITAL;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
    }
}

void Respawn(World *world, int id, CompVital *vital)
{
    vital->health    = vital->maxHealth;
    vital->energy    = vital->maxEnergy;
    vital->mana      = vital->maxMana;
    CompXform *xform = &world->xforms[id];
    if (xform)
        xform->pos.x = 0;
    xform->pos.y = 5.0f;
    xform->pos.z = 0;
}

void Damage(World *world, int id, CompVital *vital, SolHit hit)
{
    vital->health -= hit.damage;
    if (vital->health <= 0)
    {
        vital->health = 0;
        Die(world, id, vital);
    }
}

void Die(World *world, int id, CompVital *vital)
{
}
