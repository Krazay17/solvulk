/*
 * File: ability_state_claw.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "world.h"
#include "estate.h"
#include "sol_core.h"
#include "sol_math.h"
#include "render/render.h"

#define HITDELAY 0.15f
#define HITINTERVAL 0.025f
#define MELEE_RANGE 2.0f
#define DASH_SPEED 30.0f

static void Spell(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
    if (!Sol_Comp_Has(world, id, ScCombat))
        return;

    ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
    ScBody3 *body    = Sol_Comp_Get(world, id, ScBody3);

    combat->hitPause = fmaxf(0, combat->hitPause - dt * 5.0f);
    if (combat->hitPause == 0)
        data->elapsed += dt;

    if (data->elapsed >= data->conf.duration)
    {
        Sol_Ability_SetState(world, id, 0, ability->activeSlot, 1);
        return;
    }

    if (data->elapsed < HITDELAY)
        return;

    data->accum += dt;

    if (data->accum >= HITINTERVAL)
    {
        data->accum -= HITINTERVAL;
        vec3s head = Sol_Body3_GetHead(world, id);
        SolRay ray = {
            .start     = head,
            .dir       = cmd->aimdir,
            .dist      = body->dims.x + MELEE_RANGE,
            .ignoreEnt = id,
            .mask      = COLLAYER_ALL,
        };
        SolRayResult results[8];
        int hits = Sol_Raycast(world, ray, results, 8);
        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];
            vec3s hit_pos       = Sol_AddScaledDir(ray.start, ray.dir, result.t);
            float dot           = glms_vec3_dot(cmd->aimdir, glms_vec3_normalize(glms_vec3_sub(hit_pos, head)));
            if (dot < 0)
                continue;
            if (!Sol_Hitgen_Try(world, id, result.entId, data->hitgen))
                continue;

            SolHit hit = {
                .damage     = data->conf.damage,
                .buffMask   = data->conf.buffMask,
                .effectMask = data->conf.effectMask,
                .power      = 1.0f,
                .entA       = id,
                .entB       = result.entId,
                .pos        = hit_pos,
                .vel        = cmd->aimdir,
            };

            Sol_Combat_Hit(world, result.entId, hit);
            if (combat->hitPauseDiminish < 4)
            {
                combat->hitPause = 1.0f;
                combat->hitPauseDiminish++;
            }
            body->vel.y = fmaxf(body->vel.y, 1.0f);
        }
    }
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

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
    if (!Sol_Comp_Has(world, id, ScCombat))
        return;

    ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
    ScBody3 *body    = Sol_Comp_Get(world, id, ScBody3);

    combat->hitPause = fmaxf(0, combat->hitPause - dt * 5.0f);
    if (combat->hitPause == 0)
        data->elapsed += dt;

    if (data->elapsed >= data->conf.duration)
    {
        Sol_Ability_SetState(world, id, 0, ability->activeSlot, 1);
        return;
    }

    if (data->elapsed < HITDELAY)
        return;

    data->accum += dt;

    if (data->accum >= HITINTERVAL)
    {
        data->accum -= HITINTERVAL;
        vec3s head = Sol_Body3_GetHead(world, id);
        SolRay ray = {
            .start     = head,
            .dir       = cmd->aimdir,
            .dist      = body->dims.x + MELEE_RANGE,
            .ignoreEnt = id,
            .mask      = COLLAYER_ALL,
        };
        SolRayResult results[8];
        int hits = Sol_Raycast(world, ray, results, 8);
        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];
            vec3s hit_pos       = Sol_AddScaledDir(ray.start, ray.dir, result.t);
            float dot           = glms_vec3_dot(cmd->aimdir, glms_vec3_normalize(glms_vec3_sub(hit_pos, head)));
            if (dot < 0)
                continue;
            if (!Sol_Hitgen_Try(world, id, result.entId, data->hitgen))
                continue;

            SolHit hit = {
                .damage     = data->conf.damage,
                .buffMask   = data->conf.buffMask,
                .effectMask = data->conf.effectMask,
                .power      = 1.0f,
                .entA       = id,
                .entB       = result.entId,
                .pos        = hit_pos,
                .vel        = cmd->aimdir,
            };

            Sol_Combat_Hit(world, result.entId, hit);
            if (combat->hitPauseDiminish < 4)
            {
                combat->hitPause = 1.0f;
                combat->hitPauseDiminish++;
            }
            body->vel.y = fmaxf(body->vel.y, 1.0f);
        }
    }
}

void Ability_Claw_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->conf             = Sol_Ability_GetSlotConf(ability, ability->activeSlot);

    data->accum  = HITINTERVAL;
    data->hitgen = Sol_Hitgen_Start(world, id);

    data->cooldownRemaining = data->conf.cooldown;
    data->hitgen            = Sol_Hitgen_Start(world, id);
}

void Ability_Claw_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    Sol_Anim_Stop(world, id, ANIM_LAYER_UPPER, 0);
}

bool Ability_Claw_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    return !data->recoverRemaining;
}

bool Ability_Claw_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];

    return data->cooldownRemaining <= 0.0f;
}

void Ability_Claw_Draw(World *world, int id, ScAbility *ability, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
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