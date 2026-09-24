/*
 * File: s_combat.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-11
 *
 */
#include "world.h"
#include "sol_math.h"
#include "sol_core.h"

#define DESTROY_TIMER 3.0f

static void OnRespawn(World *world, int id, ScCombat *combat)
{
    Sol_Xform_Teleport(world, id, combat->respawnPos);
    combat->health  = combat->healthMax;
    combat->energy  = combat->energyMax;
    combat->mana    = combat->manaMax;
    combat->is_dead = false;
}

static void OnDeath(World *world, int id, ScCombat *combat, u32 kind)
{
    if (!combat->is_dead)
    {
        combat->is_dead   = true;
        combat->deathTime = world->tickTime;
        Sol_Event_Push(world, EVENTKIND_DEATH, (SolEvent){.entA = combat->lastHitBy, .entB = id});
    }
    Sol_Comp_Rem(world, id, ScBuff);
    ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
    if (body3)
    {
        body3->vel = GLMS_VEC3_ZERO;
    }
    if (Sol_Comp_Has(world, id, ScAbility))
    {
        Sol_Ability_SetState(world, id, 0, 0, true);
    }
    if (combat->respawnTime == 0.0f && world->tickTime >= (combat->deathTime + DESTROY_TIMER))
    {
        Sol_Destroy_Ent(world, id);
        if (body3)
            body3->flag_destroy = true;
    }
}

void Combat_Step(World *world, double dt)
{
    SparseSet_ScCombat *set = Sol_Comp_Set(world, ScCombat);
    for (int i = 0; i < set->cnt; i++)
    {
        int id           = set->dense[i];
        ScCombat *combat = &set->data[i];

        if (combat->health <= 0)
        {
            OnDeath(world, id, combat, 0);
            if (combat->respawnTime && (world->tickTime >= (combat->deathTime + combat->respawnTime)))
                OnRespawn(world, id, combat);
        }
        vec3s pos = world->xform.pos[id];
        if (pos.y < -15.0f)
        {
            combat->health = 0;
        }
    }
}

bool Sol_Combat_Hostile(World *world, int idA, int idB)
{
    if (idA == idB)
        return false;
    ScOwner *ownerA = Sol_Comp_Get(world, idA, ScOwner);
    ScOwner *ownerB = Sol_Comp_Get(world, idB, ScOwner);
    int final_A     = ownerA ? ownerA->ownerId : idA;
    int final_B     = ownerB ? ownerB->ownerId : idB;
    if (final_A == final_B)
        return false;
    ScTeam *teamA = Sol_Comp_Get(world, final_A, ScTeam);
    ScTeam *teamB = Sol_Comp_Get(world, final_B, ScTeam);
    if (teamA && teamB && teamA->team != 0 && teamB->team != 0 && teamA->team == teamB->team)
        return false;

    return true;
}

float Sol_Combat_Hit(World *world, int id, SolHit hit)
{
    if (!Sol_Comp_Has(world, id, ScCombat))
        return 0.0f;
    ScCombat *dealer_combat = Sol_Comp_Get(world, hit.entA, ScCombat);
    ScCombat *combat        = Sol_Comp_Get(world, id, ScCombat);
    float damage_done       = 0;
    float damage            = hit.damage * hit.power;
    if (hit.isHeal)
        damage_done = Sol_Combat_Heal(world, id, hit.entA, combat, damage);
    else if (Sol_Combat_Hostile(world, hit.entA, id) && !Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN))
    {
        if (!hit.isHeal)
        {
            if (hit.buffMask > 0)
            {
                Sol_Buff_AddMask(world, id, hit.buffMask, hit.entA, hit.power);
            }
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
                if (dealer_combat)
                    Sol_Combat_Heal(world, hit.entA, hit.entA, dealer_combat, damage * 0.2f);
            }
            if (hit.effectMask & EFFECTMASK_REFLECTPROJECTILE && Sol_Comp_Has(world, id, ScProjectile))
            {
                ScProjectile *projectile = Sol_Comp_Get(world, id, ScProjectile);
                ScTeam *team             = Sol_Comp_Get(world, id, ScTeam);
                ScTeam *attacker_team    = Sol_Comp_Get(world, hit.entA, ScTeam);
                ScOwner *owner           = Sol_Comp_Get(world, id, ScOwner);
                if (owner)
                    owner->ownerId = hit.entA;
                if (team)
                    team->team = attacker_team->team;

                ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
                ScCmd *cmd     = Sol_Comp_Get(world, hit.entA, ScCmd);
                if (body3 && cmd)
                {
                    body3->vel = Sol_RedirectVel(body3->vel, cmd->aimdir);
                }
                Sol_Event_Push(world, EVENTKIND_FX, (SolEvent){.as.fx.kind = FXKIND_PARRY, .as.fx.pos = hit.pos});
                return 0.0f;
            }
        }
        combat->lastHitBy = hit.entA;
        damage_done       = Sol_Combat_Damage(world, id, hit.entA, combat, damage);

        Sol_Event_Push(world, EVENTKIND_HIT, (SolEvent){.entA = hit.entA, .entB = id, .as.hit = hit});
    }
    else
    {
        damage_done = 0;
        Sol_Event_Push(world, EVENTKIND_FX, (SolEvent){.as.fx.kind = FXKIND_INVULNHIT, .as.fx.pos = hit.pos});
    }

    return damage_done;
}

float Sol_Combat_Damage(World *world, int id, int source, ScCombat *combat, float amount)
{
    if (combat->health <= 0.0f || amount <= 0.0f)
        return 0.0f;

    float damage_done = (amount > combat->health) ? combat->health : amount;
    combat->health -= damage_done;
    combat->damageTaken += damage_done;
    combat->lastHitTime = world->tickTime;

    ScCombat *combatA = Sol_Comp_Get(world, source, ScCombat);
    if (combatA)
        combatA->damageDone += damage_done;

    return damage_done;
}

float Sol_Combat_Heal(World *world, int id, int dealer, ScCombat *combat, float amount)
{
    if (combat->health <= 0.0f || amount <= 0.0f)
        return 0.0f;

    float missing_health = combat->healthMax - combat->health;
    float healing_done   = (amount > missing_health) ? missing_health : amount;
    combat->health += healing_done;
    combat->healingTaken += healing_done;

    ScCombat *combatA = Sol_Comp_Get(world, dealer, ScCombat);
    if (combatA)
        combatA->healingDone += healing_done;

    return healing_done;
}

void Sol_Combat_DamageSphere(World *world, int id, SolRay ray, SolHit hit, SolRayResult *results, int max_hits)
{
    vec3s pos = ray.start;
    int hits  = Sol_SphereOverlap(world, ray, results, max_hits);

    for (int i = 0; i < hits; i++)
    {
        int hit_id = results[i].entId;
        if (hit_id == id || Sol_Comp_Has(world, hit_id, ScStage))
            continue;

        vec3s hit_pos = world->xform.pos[hit_id];
        vec3s delta   = vecSub(hit_pos, pos);
        float d2      = glms_vec3_norm2(delta);

        // Only run LoS raycast if entity isn't sitting directly on the explosion origin
        if (d2 >= 0.000001f)
        {
            float dist = sqrtf(d2);
            vec3s dir  = vecSca(delta, 1.0f / dist);

            SolRayResult los_result = {0};
            bool is_blocked =
                Sol_Raycast1(world,
                             (SolRay){.start     = pos,
                                      .dir       = dir,
                                      .dist      = dist - 0.01f, // Stop slightly short to avoid self-intersection
                                      .mask      = COLLAYER_WORLD,
                                      .ignoreEnt = id},
                             &los_result);

            if (is_blocked)
                continue;
        }
        hit.entB = hit_id;
        hit.pos  = hit_pos;
        Sol_Combat_Hit(world, hit_id, hit);
    }
}

int Sol_Combat_DamageCast(World *world, int id, SolRay ray, SolHit hit, u32 hitgen)
{
    SolRayResult results[64];
    int max_hits = 64;

    int hits = solState.debug ? Sol_SpherecastD(world, ray, results, max_hits, 0.2f)
                              : Sol_Spherecast(world, ray, results, max_hits);
    for (int i = 0; i < hits; i++)
    {
        int hit_id = results[i].entId;
        if (hit_id == id || Sol_Comp_Has(world, hit_id, ScStage) || !Sol_Comp_Has(world, hit_id, ScCombat))
            continue;

        if (!Sol_Hitgen_Try(world, id, hit_id, hitgen))
            continue;

        if (!hit.isHeal && !Sol_Combat_Hostile(world, id, hit_id))
            continue;

        vec3s hit_pos = world->xform.pos[hit_id];
        vec3s delta   = vecSub(hit_pos, ray.start);
        float d2      = glms_vec3_norm2(delta);

        if (d2 >= 0.000001f)
        {
            float dist = sqrtf(d2);
            vec3s dir  = vecSca(delta, 1.0f / dist);

            SolRayResult los_result = {0};
            bool is_blocked =
                Sol_Raycast1(world,
                             (SolRay){.start     = ray.start,
                                      .dir       = dir,
                                      .dist      = dist - 0.01f, // Stop slightly short to avoid self-intersection
                                      .mask      = COLLAYER_WORLD,
                                      .ignoreEnt = id},
                             &los_result);
            if (is_blocked)
                continue;
        }
        hit.entB = hit_id;
        hit.pos  = hit_pos;
        Sol_Combat_Hit(world, hit_id, hit);
    }
    return hits;
}