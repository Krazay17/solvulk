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
#include "physx/s_body.h"

const CompCombat vital_config[] = {
    [COMBATKIND_PLAYER] =
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
    [COMBATKIND_WIZARD] =
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

    CompCombat *vital = &world->combats[id];
    vital->deathTime = solState.gameTime;
    world->masks[id] &= ~BITC(HAS_BUFF);

    Sol_Item_Drop(world, id);

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.kind = FXKIND_DEATH_BLOOD,
                             .as.fx.pos  = Sol_Xform_GetPos(world, id),
                         });
    world->bodies[id].group = PHYSXMASK(0, 1);
}

static void OnRespawn(World *world, int id)
{
    CompCombat *vital = &world->combats[id];
    vital->health    = vital->maxHealth;
    vital->energy    = vital->maxEnergy;
    vital->mana      = vital->maxMana;
    Sol_Xform_Teleport(world, id, vital->respawnPos);
    Sol_Movement_SetState(world, id, MOVE_IDLE);
    world->bodies[id].group = world->bodies[id].base_group;
}

static int  required_step = BITC(HAS_COMBAT);
static void Vital_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, required_step))
            continue;
        CompCombat *vital = &world->combats[id];
        if (vital->health == 0 && vital->doesRespawn && time > vital->deathTime + vital->respawnTime)
            OnRespawn(world, id);
    }
}

void Sol_Vital_Init(World *world)
{
    world->combats   = calloc(MAX_ENTS, sizeof(CompCombat));
    WAddStep(world) = Vital_Step;
}

void Sol_Combat_Add(World *world, int id, VitalKind kind)
{
    CompCombat vital   = vital_config[kind];
    vital.lastHitTime = -FLT_MAX;
    vital.respawnPos  = Sol_Xform_GetPos(world, id);
    world->masks[id] |= BITC(HAS_COMBAT);
    world->combats[id] = vital;
}

float Sol_Vital_Damage(World *world, int id, int attacker, float damage)
{
    if (!(world->masks[id] & BITC(HAS_COMBAT)))
        return 0;
    CompCombat *vital       = &world->combats[id];
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

// pass in amount as percent
float Sol_Vital_HealPercent(World *world, int id, int dealer, float amount)
{
    if (!(world->masks[id] & BITC(HAS_COMBAT)))
        return 0;
    CompCombat *vital       = &world->combats[id];
    float      amountDealt = 0.0f;
    if (vital->health == 0)
        return 0;
    float finalAmount = vital->maxHealth * amount;

    if (vital->health + finalAmount >= vital->maxHealth)
    {
        vital->health = vital->maxHealth;
        amountDealt   = vital->maxHealth - vital->health;
    }
    else
    {
        vital->health += finalAmount;
        amountDealt = finalAmount;
    }

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.entA = dealer,
                             .as.fx.entB = id,
                             .as.fx.pos  = Sol_Xform_GetPos(world, id),
                             .as.fx.kind = FXKIND_TAKEHEALING,
                         });

    return amountDealt;
}

float Sol_Vital_Heal(World *world, int id, int dealer, float amount)
{
    if (!(world->masks[id] & BITC(HAS_COMBAT)))
        return 0;
    CompCombat *vital       = &world->combats[id];
    float      amountDealt = 0.0f;
    if (vital->health == 0)
        return 0;

    if (vital->health + amount >= vital->maxHealth)
    {
        vital->health = vital->maxHealth;
        amountDealt   = vital->maxHealth - vital->health;
    }
    else
    {
        vital->health += amount;
        amountDealt = amount;
    }

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.entA = dealer,
                             .as.fx.entB = id,
                             .as.fx.pos  = Sol_Xform_GetPos(world, id),
                             .as.fx.kind = FXKIND_TAKEHEALING,
                         });

    return amountDealt;
}

float Sol_Combat_GetHealth(World *world, int id)
{
    if (!(world->masks[id] & BITC(HAS_COMBAT)))
        return 1;
    return world->combats[id].health;
}
bool Sol_Combat_GetDead(World *world, int id)
{
    return (world->masks[id] & BITC(HAS_COMBAT)) && (world->combats[id].health == 0);
}
