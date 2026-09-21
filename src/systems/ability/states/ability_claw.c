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

void Ability_Claw_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

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
                .power = 1.0f,
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

            Sol_Event_Push(world, EVENTKIND_FX,
                           (SolEvent){
                               .as.fx.pos  = hit_pos,
                               .as.fx.kind = EVENTFX_CLAW_HIT,
                           });
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

}

void Ability_Claw_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    Sol_Anim_Stop(world, id, ANIM_LAYER_UPPER, 0);
}

bool Ability_Claw_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    return data->elapsed >= data->conf.duration * 0.8f;
}

bool Ability_Claw_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];

    return data->cooldownRemaining <= 0.0f;
}

void Ability_Claw_Draw(World *world, int id, ScAbility *ability)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
}

const AbilityStateFunc ability_claw_state = {
    .update   = Ability_Claw_Update,
    .enter    = Ability_Claw_Enter,
    .exit     = Ability_Claw_Exit,
    .canExit  = Ability_Claw_CanExit,
    .canEnter = Ability_Claw_CanEnter,
    .draw     = Ability_Claw_Draw,
};