/*
 * File: s_combat.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-11
 *
 */
#include "world.h"
#include "sol_math.h"

#define DESTROY_TIMER 3.0f

static void OnRespawn(World *world, int id, ScCombat *combat)
{
    combat->health = combat->healthMax;
    combat->energy = combat->energyMax;
    combat->mana   = combat->manaMax;
}

static void OnDeath(World *world, int id, ScCombat *combat)
{
    if (!combat->is_dead)
    {
        combat->is_dead   = true;
        combat->deathTime = world->tickTime;
    }
    if (combat->respawnTime == 0.0f && world->tickTime >= (combat->deathTime + DESTROY_TIMER))
        Sol_Destroy_Ent(world, id);

    ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
    if (body3)
        body3->flag_destroy = true;
    if (Sol_Comp_Has(world, id, ScAbility))
    {
        Sol_Ability_SetState(world, id, 0, 0, true);
    }
}

void Combat_Step(World *world)
{
    SparseSet_ScCombat *set = Sol_Comp_Set(world, ScCombat);
    for (int i = 0; i < set->cnt; i++)
    {
        int id           = set->dense[i];
        ScCombat *combat = &set->data[i];

        if (combat->health <= 0)
        {
            OnDeath(world, id, combat);
            if (combat->respawnTime && (world->tickTime >= (combat->deathTime + combat->respawnTime)))
                OnRespawn(world, id, combat);
        }
    }
}

float Sol_Combat_Hit(World *world, int id, SolHit hit)
{
    if (!Sol_Comp_Has(world, id, ScCombat))
        return 0.0f;
    ScCombat *combat  = Sol_Comp_Get(world, id, ScCombat);
    float damage_done = 0;
    float damage      = hit.damage;
    if (!hit.isHeal)
    {
        if (hit.effectMask & EFFECTMASK_KNOCKBACK)
        {
            ScMove3 *move3     = Sol_Comp_Get(world, id, ScMove3);
            move3->knockVel    = vecSca(hit.vel, 10.0f);
            move3->knockDur    = 0.4f;
            move3->frictionMod = 0.0f;
        }
        if (hit.effectMask & EFFECTMASK_KNOCKUP)
        {
            ScMove3 *move3     = Sol_Comp_Get(world, id, ScMove3);
            move3->knockVel    = vecSca(WORLD_UP, 10.0f);
            move3->knockDur    = 0.4f;
            move3->frictionMod = 0.0f;
        }
        if (hit.effectMask & EFFECTMASK_LIFESTEAL)
        {
            Sol_Combat_Heal(world, hit.entA, Sol_Comp_Get(world, hit.entA, ScCombat), damage * 0.2f);
        }
        if (hit.effectMask & EFFECTMASK_REFLECTPROJECTILE && Sol_Comp_Has(world, id, ScProjectile))
        {
            ScProjectile *projectile = Sol_Comp_Get(world, id, ScProjectile);
            ScTeam *team             = Sol_Comp_Get(world, id, ScTeam);
            ScTeam *attacker_team    = Sol_Comp_Get(world, hit.entA, ScTeam);

            team->team = attacker_team->team;
        }
    }

    if (hit.isHeal)
        damage_done = Sol_Combat_Heal(world, id, combat, damage);
    else
        damage_done = Sol_Combat_Damage(world, id, combat, damage);

    ScCombat *combatA = Sol_Comp_Get(world, hit.entA, ScCombat);
    if (combatA)
        combatA->damageDone += damage_done;

    return damage_done;
}

float Sol_Combat_Damage(World *world, int id, ScCombat *combat, float amount)
{
    if (combat->health <= 0.0f || amount <= 0.0f)
        return 0.0f;

    float damage_done = (amount > combat->health) ? combat->health : amount;
    combat->health -= damage_done;
    combat->damageTaken += damage_done;
    combat->lastHitTime = world->tickTime;

    return damage_done;
}

float Sol_Combat_Heal(World *world, int id, ScCombat *combat, float amount)
{
    if (combat->health <= 0.0f || amount <= 0.0f)
        return 0.0f;

    float missing_health = combat->healthMax - combat->health;
    float healing_done   = (amount > missing_health) ? missing_health : amount;
    combat->health += healing_done;
    combat->healingTaken += healing_done;

    return healing_done;
}
