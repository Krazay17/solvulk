/*
 * File: s_vital.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Vitals!
 */
#include "sol_core.h"

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
    world->stepSystems[world->stepCount++]     = Sol_Vital_Step;
    world->draw3dSystems[world->draw3dCount++] = Sol_Vital_Draw;

    world->vitals = calloc(MAX_ENTS, sizeof(CompVital));
}

void Sol_Vital_Add(World *world, int id, VitalDesc desc)
{
    world->masks[id] |= HAS_VITAL;
    CompVital vital = {
        .maxHealth = desc.maxHealth,
        .maxEnergy = desc.maxEnergy,
        .maxMana   = desc.maxMana,
        .health    = 50,
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
        SolXform xform = Sol_Xform_GetDrawXform(world, id);
        xform.pos.y += Sol_Physx_GetDims(world, id).y - 1.0f;

        CompVital *vital = &world->vitals[id];
        float      fill  = vital->maxHealth > 0 ? (float)vital->health / (float)vital->maxHealth : 0.0f;

        Sol_Render_PushBillboard((BillboardDesc){
            .kind   = BILLBOARD_HEALTHBAR,
            .pos    = (vec4s){{xform.pos.x, xform.pos.y, xform.pos.z, 1.0f}},
            .color  = (vec4s){{255.0f, 0.85f, 122.2f, 255.0f}}, // healthy green
            .params = (vec4s){{fill, 0.05f, 0.0f, 0.0f}},
        });
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
