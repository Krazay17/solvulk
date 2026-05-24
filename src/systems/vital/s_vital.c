/*
 * File: s_vital.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Vitals!
 */
#include "sol_core.h"
#include "vital.h"

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

void Sol_Vital_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Vital_Step;
    world->vitals                          = calloc(MAX_ENTS, sizeof(CompVital));
}

void Sol_Vital_Add(World *world, int id, VitalDesc desc)
{
    world->masks[id] |= HAS_VITAL;
    CompVital vital = {
        .maxHealth   = desc.maxHealth,
        .maxEnergy   = desc.maxEnergy,
        .maxMana     = desc.maxMana,
        .health      = desc.maxHealth,
        .energy      = desc.maxEnergy,
        .mana        = desc.maxMana,
        .team        = desc.team,
        .lastHitTime = -100.0f,
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

void Die(World *world, int id, SolHit hit)
{
    Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_DEATH, .as.death.attacker = hit.source, .sourceId = id});
    Sol_Destroy_Ent(world, id);
    Sol_Emitter_Add(world, (Emitter){.burst    = 40,
                                     .pos      = Sol_Xform_GetPos(world, id),
                                     .particle = (Particle){.color    = {.r = 1.0f, .g = 0, .b = 0, .a = 1.0f},
                                                            .kind     = PARTICLE_BLOOD,
                                                            .scale    = 0.4f,
                                                            .ttl      = 1.5f,
                                                            .speed    = 2.5f,
                                                            .scaleout = .5f}});
}

void Sol_Vital_Damage(World *world, int id, SolHit hit)
{
    if (!(world->masks[id] & HAS_VITAL))
        return;
    CompVital *vital = &world->vitals[id];

    if (hit.damage >= vital->health)
    {
        vital->health = 0;
        Die(world, id, hit);
    }
    else
    {
        vital->health -= hit.damage;
        vital->lastHitTime = (float)Sol_GetGameTime();
    }
}

u32 Sol_Vital_GetHealth(World *world, int id)
{
    return world->vitals[id].health;
}
u32 Sol_Vital_GetMaxHealth(World *world, int id)
{
    return world->vitals[id].maxHealth;
}

u32 Sol_Vital_GetTeam(World *world, int id)
{
    return world->vitals[id].team;
}
u32 Sol_Vital_GetHostile(World *world, int id, int target)
{
    u32 teamA = world->vitals[id].team;
    u32 teamB = world->vitals[target].team;

    return teamA == 0 || teamA != teamB;
}
bool Sol_Vital_GetIsalive(World *world, int id)
{
    return world->vitals[id].health > 0;
}
float Sol_Vital_GetLastHitTime(World *world, int id)
{
    return world->vitals[id].lastHitTime;
}