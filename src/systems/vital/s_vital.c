/*
 * File: s_vital.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Vitals!
 */
#include "sol_core.h"
#include "vital.h"

const CompVital vital_config[] = {
    [VITALKIND_PLAYER] =
        {
            .maxHealth   = 100,
            .health      = 100,
            .maxEnergy   = 100,
            .energy      = 100,
            .maxMana     = 100,
            .mana        = 100,
            .doesRespawn = 1,
            .respawnTime = 1.0f,
        },
    [VITALKIND_WIZARD] =
        {
            .maxHealth = 100,
            .health    = 100,
            .maxEnergy = 100,
            .energy    = 100,
            .maxMana   = 100,
            .mana      = 100,
        },
};

void Sol_Vital_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Vital_Step;
    world->vitals                          = calloc(MAX_ENTS, sizeof(CompVital));
}

void Sol_Vital_Add(World *world, int id, VitalKind kind)
{
    CompVital vital = vital_config[kind];

    world->masks[id] |= HAS_VITAL;
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
        if (vital->health == 0)
            Die(world, id);
        if (vital->doesRespawn && vital->isDead && time > vital->deathTime + vital->respawnTime)
            Respawn(world, id, vital);
    }
}

void Respawn(World *world, int id, CompVital *vital)
{
    vital->health = vital->maxHealth;
    vital->energy = vital->maxEnergy;
    vital->mana   = vital->maxMana;
    vital->isDead = 0;

    CompXform *xform = &world->xforms[id];
    if (xform)
        xform->pos.x = 0;
    xform->pos.y = 5.0f;
    xform->pos.z = 0;
}

void Die(World *world, int id)
{
    CompVital *vital = &world->vitals[id];
    if (vital->isDead)
        return;
    vital->isDead = 1;

    vital->deathTime = Sol_GetGameTime();

    if (world->masks[id] & HAS_BUFF)
        memset(&world->buffs[id], 0, sizeof(CompBuff));

    Sol_Emitter_Add(world, (Emitter){.burst    = 40,
                                     .pos      = Sol_Xform_GetPos(world, id),
                                     .particle = (Particle){.color    = {.r = 1.0f, .g = 0, .b = 0, .a = 1.0f},
                                                            .kind     = PARTICLE_BLOOD,
                                                            .scale    = 0.4f,
                                                            .ttl      = 1.5f,
                                                            .speed    = 2.5f,
                                                            .scaleout = .5f}});
    if (!vital->doesRespawn)
        Sol_Destroy_Ent(world, id);
}

void Sol_Vital_Damage(World *world, int id, const SolHit *hit)
{
    if (!(world->masks[id] & HAS_VITAL))
        return;
    CompVital *vital = &world->vitals[id];

    if (vital->isDead)
        return;

    if (hit->damage >= vital->health)
    {
        vital->health = 0;
        Die(world, id);
        Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_DEATH, .as.death.entA = hit->entA, .as.death.entB = id});
    }
    else
    {
        vital->health -= hit->damage;
        vital->lastHitTime = Sol_GetGameTime();
    }
}

u32 Sol_Vital_GetHealth(World *world, int id)
{
    if (!(world->masks[id] & HAS_VITAL))
        return 1;
    return world->vitals[id].health;
}
u32 Sol_Vital_GetMaxHealth(World *world, int id)
{
    return world->vitals[id].maxHealth;
}
bool Sol_Vital_GetDead(World *world, int id)
{
    return world->masks[id] & HAS_VITAL && (world->vitals[id].health == 0 || world->vitals[id].isDead);
}
float Sol_Vital_GetLastHitTime(World *world, int id)
{
    return world->vitals[id].lastHitTime;
}