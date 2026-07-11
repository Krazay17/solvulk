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
#include "movement/s_movement.h"
#include "ability/s_ability.h"
#include "item/s_item.h"

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

static void OnDeath(World *world, int id)
{
    Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, -1, true);
    Sol_Movement_ForceState(world, id, MOVE_DEAD);

    CompVital *vital = &world->vitals[id];
    vital->deathTime = solState.gameTime;
    world->masks[id] &= ~BITC(HAS_BUFF);

    Sol_Item_Drop(world, id);

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.kind = FXKIND_DEATH_BLOOD,
                             .as.fx.pos  = Sol_Xform_GetPos(world, id),
                         });
}

static void OnRespawn(World *world, int id)
{
    CompVital *vital = &world->vitals[id];
    vital->health    = vital->maxHealth;
    vital->energy    = vital->maxEnergy;
    vital->mana      = vital->maxMana;
    Sol_Xform_Teleport(world, id, vital->respawnPos);
    Sol_Movement_SetState(world, id, MOVE_IDLE);
}

static int  required_step = BITC(HAS_VITAL);
static void Vital_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, required_step))
            continue;
        CompVital *vital = &world->vitals[id];
        if (vital->health == 0 && vital->doesRespawn && time > vital->deathTime + vital->respawnTime)
            OnRespawn(world, id);
    }
}

void Sol_Vital_Init(World *world)
{
    world->vitals   = calloc(MAX_ENTS, sizeof(CompVital));
    WAddStep(world) = Vital_Step;
}

void Sol_Vital_Add(World *world, int id, VitalKind kind)
{
    CompVital vital   = vital_config[kind];
    vital.lastHitTime = -FLT_MAX;
    vital.respawnPos  = Sol_Xform_GetPos(world, id);
    world->masks[id] |= BITC(HAS_VITAL);
    world->vitals[id] = vital;
}

float Sol_Vital_Damage(World *world, int id, int attacker, float damage)
{
    if (!(world->masks[id] & BITC(HAS_VITAL)))
        return 0;
    CompVital *vital       = &world->vitals[id];
    float      damageDealt = 0.0f;
    if (vital->health == 0)
        return 0;

    if (damage >= vital->health)
    {
        if (vital->health > 0)
        {
            if (!Net_IsClient())
                OnDeath(world, id);
            vital->deathTime = solState.gameTime;
        }
        damageDealt   = vital->health;
        vital->health = 0;
    }
    else
    {
        vital->health -= damage;
        vital->lastHitTime = solState.gameTime;
        damageDealt        = damage;
    }

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.entA = attacker,
                             .as.fx.entB = id,
                             .as.fx.pos  = Sol_Xform_GetPos(world, id),
                             .as.fx.kind = FXKIND_TAKEDAMAGE,
                         });

    return damageDealt;
}

void Sol_Vital_Heal(World *world, int id, int healer, u32 heal)
{
    if (!(world->masks[id] & BITC(HAS_VITAL)))
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
    if (!(world->masks[id] & BITC(HAS_VITAL)))
        return 1;
    return world->vitals[id].health;
}
float Sol_Vital_GetMaxHealth(World *world, int id)
{
    return world->vitals[id].maxHealth;
}
bool Sol_Vital_GetDead(World *world, int id)
{
    return (world->masks[id] & BITC(HAS_VITAL)) && (world->vitals[id].health == 0);
}
float Sol_Vital_GetLastHitTime(World *world, int id)
{
    return world->vitals[id].lastHitTime;
}
