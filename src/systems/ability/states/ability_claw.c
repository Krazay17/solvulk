/*
 * File: ability_claw.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "world.h"
#include "estate.h"
#include "sol_core.h"
#include "sol_math.h"

#include "prefabs.h"
#include "render/render.h"

#define HITDELAY 0.2f
#define HITINTERVAL 0.05f
#define MELEE_RANGE 2.0f
#define DASH_SPEED 30.0f

static vec3s GetProjectilePos(World *world, int id, float power, int slot)
{
    vec3s pos = Sol_Model_GetBoneXform(world, id, slot == 1 ? "hand.R" : "hand.L").pos;
    if (glms_vec3_norm2(pos) == 0.0f)
        pos = Sol_Body3_GetHead(world, id);
    pos = vecAdd(pos, vecSca(WORLD_UP, (power * 0.5f)));
    return pos;
}

static void Spell(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    // AbilityStateData *data = &ability->stateData[ability->activeSlot];
    // data->elapsed += dt;
    // if (!Sol_Comp_Has(world, id, ScCombat))
    //     return;

    // ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);

    // combat->hitPause = fmaxf(0, combat->hitPause - dt * 5.0f);
    // if (combat->hitPause == 0)
    //     data->elapsed += dt;

    // if (data->elapsed >= data->conf.duration)
    // {
    //     Sol_Ability_SetState(world, id, 0, ability->activeSlot, 1);
    //     return;
    // }

    // if (data->elapsed < HITDELAY)
    //     return;

    // ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
    // data->accum += dt;
    // if (data->accum >= HITINTERVAL)
    // {
    //     data->accum -= HITINTERVAL;
    //     vec3s head = Sol_Body3_GetHead(world, id);
    //     SolRay ray = {
    //         .start     = head,
    //         .dir       = cmd->aimdir,
    //         .dist      = body->dims.x + MELEE_RANGE,
    //         .ignoreEnt = id,
    //         .mask      = COLLAYER_ALL,
    //     };
    //     SolRayResult results[8];
    //     int hits = Sol_Raycast(world, ray, results, 8);
    //     for (int i = 0; i < hits; i++)
    //     {
    //         SolRayResult result = results[i];
    //         vec3s hit_pos       = Sol_AddScaledDir(ray.start, ray.dir, result.t);
    //         float dot           = glms_vec3_dot(cmd->aimdir, glms_vec3_normalize(glms_vec3_sub(hit_pos, head)));
    //         if (dot < 0)
    //             continue;
    //         if (!Sol_Hitgen_Try(world, id, result.entId, data->hitgen))
    //             continue;

    //         SolHit hit = {
    //             .damage     = data->conf.damage,
    //             .buffMask   = data->conf.buffMask,
    //             .effectMask = data->conf.effectMask,
    //             .power      = 1.0f,
    //             .entA       = id,
    //             .entB       = result.entId,
    //             .pos        = hit_pos,
    //             .vel        = cmd->aimdir,
    //         };

    //         Sol_Combat_Hit(world, result.entId, hit);
    //         if (combat->hitPauseDiminish < 4)
    //         {
    //             combat->hitPause = 1.0f;
    //             combat->hitPauseDiminish++;
    //         }
    //         body->vel.y = fmaxf(body->vel.y, 1.0f);
    //     }
    // }
}

static void Dash(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
    ScBody3 *body3       = Sol_Comp_Get(world, id, ScBody3);
    vec3s pos            = world->xform.pos[id];
    float duration_delta = data->elapsed / data->conf.duration;
    if (body3)
    {
        body3->vel = vecSca(cmd->lookdir, Sol_Math_MapRange(DASH_SPEED, 6.0f, 0, 1.0f, duration_delta));
    }

    data->accum += dt;
    if (data->accum >= HITINTERVAL)
    {
        data->accum -= HITINTERVAL;
        Sol_Combat_DamageCast(world, id,
                              (SolRay){
                                  .start     = pos,
                                  .dir       = cmd->lookdir,
                                  .dist      = 1.0f,
                                  .ignoreEnt = id,
                                  .radius    = body3->dims.y,
                              },
                              (SolHit){
                                  .entA       = id,
                                  .damage     = data->conf.damage,
                                  .buffMask   = data->conf.buffMask,
                                  .effectMask = data->conf.effectMask,
                                  .kind       = HITKIND_NORMAL,
                              },
                              data->hitgen);
    }
}

static void Hook_OnHit(World *world, int a, int b)
{
    ScAbility *ability     = Sol_Comp_Get(world, a, ScAbility);
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    if (data->hitPauseDr < 4)
    {
        data->hitPause = 1.0f;
        data->hitPauseDr++;
    }
    ScBody3 *body = Sol_Comp_Get(world, a, ScBody3);
    if (body)
        body->vel.y = fmaxf(body->vel.y, 1.0f);
}

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    switch (data->stage)
    {
    case 0:
        if (!data->held)
        {
            data->stage++;
            goto fire;
        }
        data->power = min(data->conf.maxpower, data->power + (dt * data->conf.chargespeed));
        break;
    case 1:
    fire:
        data->stage++;
        vec3s pos = GetProjectilePos(world, id, data->power, ability->activeSlot);
        vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));

        SolHit hit = {
            .entA   = id,
            .power  = data->power,
            .damage = data->conf.damage,
        };

        { // Spawn fireball
            int fireball             = Sol_Prefab_Fireball(world, id, pos, dir, 25.0f, data->power);
            ScBody3 *pBody           = Sol_Comp_Get(world, fireball, ScBody3);
            ScProjectile *projectile = Sol_Comp_Get(world, fireball, ScProjectile);
            projectile->hit          = hit;
            projectile->aoe_hit      = hit;
            projectile->power        = data->power;
        }
        break;
    case 2:
        if (data->elapsed > HITDELAY)
            data->stage++;
        break;
    case 3:
        data->accum += dt;
        if (data->accum >= HITINTERVAL)
        {
            data->accum -= HITINTERVAL;
            vec3s head = Sol_Body3_GetHead(world, id);
            SolRay ray = {
                .start     = head,
                .dir       = cmd->aimdir,
                .dist      = MELEE_RANGE,
                .ignoreEnt = id,
                .mask      = COLLAYER_ALL,
            };
            SolHit hit = {
                .kind       = HITKIND_MELEE_HIT,
                .damage     = data->conf.damage,
                .buffMask   = data->conf.buffMask,
                .effectMask = data->conf.effectMask,
                .power      = 1.0f,
                .entA       = id,
                .vel        = cmd->aimdir,
                .hook       = Hook_OnHit,
            };
            Sol_Combat_DamageCast(world, id, ray, hit, data->hitgen);
        }
        break;
    }

    if (data->elapsed >= data->conf.duration)
    {
        Sol_Ability_SetState(world, id, 0, ability->activeSlot, 1);
        return;
    }
    if (data->stage > 0)
    {
        if (data->hitPause > 0)
            data->hitPause = fmaxf(0, data->hitPause - dt * 5.0f);
        else
            data->elapsed += dt;
    }
}

void Ability_Claw_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->conf             = Sol_Ability_GetSlotConf(ability, ability->activeSlot);

    data->accum             = HITINTERVAL;
    data->hitgen            = Sol_Hitgen_Start(world, id);
    data->cooldownRemaining = data->conf.cooldown;
}

void Ability_Claw_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    // Sol_Anim_Stop(world, id, ANIM_LAYER_UPPER, 0);
}

bool Ability_Claw_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    return true;
}

bool Ability_Claw_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];

    return data->cooldownRemaining <= 0.0f;
}

void Ability_Claw_Draw(World *world, int id, ScAbility *ability, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    Xform hand_xform       = Sol_Model_GetBoneXform(world, id, ability->activeSlot == 1 ? "hand.R" : "hand.L");
    vec4s pos              = (vec4s){hand_xform.pos.x, hand_xform.pos.y, hand_xform.pos.z, data->power};
    if (data->stage == 0)
    {
        ModelSSBO *s = Sol_Render_GetNextModel(0, MODELKIND_WEAPONBLADE);
        s->color     = (vec4s){1, 1, 1, 1};
        s->position  = pos;
        s->scale     = (vec4s){data->power, data->power, data->power, data->power};
        s->rotation  = (vec4s){hand_xform.rot.x, hand_xform.rot.y, hand_xform.rot.z, hand_xform.rot.w};
    }
}

const AbilityStateFunc ability_claw_state = {
    .update   = Spell,
    .enter    = Ability_Claw_Enter,
    .exit     = Ability_Claw_Exit,
    .canExit  = Ability_Claw_CanExit,
    .canEnter = Ability_Claw_CanEnter,
    .draw     = Ability_Claw_Draw,
};

const AbilityStateFunc ability_claw_charge_state = {
    .update   = Charge,
    .enter    = Ability_Claw_Enter,
    .exit     = Ability_Claw_Exit,
    .canExit  = Ability_Claw_CanExit,
    .canEnter = Ability_Claw_CanEnter,
    .draw     = Ability_Claw_Draw,
};

const AbilityStateFunc ability_claw_dash_state = {
    .update   = Dash,
    .enter    = Ability_Claw_Enter,
    .exit     = Ability_Claw_Exit,
    .canExit  = Ability_Claw_CanExit,
    .canEnter = Ability_Claw_CanEnter,
    .draw     = Ability_Claw_Draw,
};