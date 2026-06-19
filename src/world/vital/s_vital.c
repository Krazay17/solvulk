/*
 * File: s_vital.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Vitals!
 */
#include "s_vital.h"
#include "sol_core.h"
#include "world.h"
#include "network.h"
#include "xform/s_xform.h"
#include "event/s_event.h"

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
            .respawnTime = 2.0f,
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
    world->vitals = calloc(MAX_ENTS, sizeof(CompVital));
}

void Sol_Vital_Add(World *world, int id, VitalKind kind)
{
    CompVital vital   = vital_config[kind];
    vital.lastHitTime = -FLT_MAX;
    world->masks[id] |= HAS_VITAL;
    world->vitals[id] = vital;
}

void Sol_Vital_Damage(World *world, int id, int attacker, float damage)
{
    if (!(world->masks[id] & HAS_VITAL))
        return;
    CompVital *vital = &world->vitals[id];

    if (damage >= vital->health)
    {
        if (vital->health > 0)
        {
            if (!Net_IsClient())
                Sol_Event_Add(world,
                              (SolEvent){.kind = EVENTKIND_DEATH, .as.death.entA = attacker, .as.death.entB = id});
            Sol_Event_Add(world, (SolEvent){
                                     .kind       = EVENTKIND_FX,
                                     .as.fx.entA = attacker,
                                     .as.fx.entB = id,
                                     .as.fx.pos  = Sol_Xform_GetPos(world, id),
                                     .as.fx.kind = FXKIND_TAKEDAMAGE,
                                 });
            vital->deathTime = solState.gameTime;
        }
        vital->health = 0;
    }
    else
    {
        vital->health -= damage;
        vital->lastHitTime = solState.gameTime;
    }
}

void Sol_Vital_Heal(World *world, int id, int healer, u32 heal)
{
    if (!(world->masks[id] & HAS_VITAL))
        return;
    CompVital *vital = &world->vitals[id];

    if (vital->health + heal >= vital->maxHealth)
    {
        vital->health = vital->maxHealth;
    }
    else
    {
        vital->health += heal;
    }
}

float Sol_Vital_GetHealth(World *world, int id)
{
    if (!(world->masks[id] & HAS_VITAL))
        return 1;
    return world->vitals[id].health;
}
float Sol_Vital_GetMaxHealth(World *world, int id)
{
    return world->vitals[id].maxHealth;
}
bool Sol_Vital_GetDead(World *world, int id)
{
    return (world->masks[id] & HAS_VITAL) && (world->vitals[id].health == 0);
}
float Sol_Vital_GetLastHitTime(World *world, int id)
{
    return world->vitals[id].lastHitTime;
}