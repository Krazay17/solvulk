/*
 * File: ability_state_claw.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "ability/s_ability.h"
#include "world.h"
#include "sol_math.h"
#include "render/render.h"

#define HITDELAY 0.2f
#define HITINTERVAL 0.1f
#define MELEE_RANGE 2.0f

void Ability_Claw_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    if (!Sol_Comp_Has(world, id, ScCombat))
        return;
    ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);

    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    ScBody3 *body          = Sol_Comp_Get(world, id, ScBody3);
    ScAnim *anim           = Sol_Comp_Get(world, id, ScAnim);

    combat->hitPause = fmaxf(0, combat->hitPause - dt * 5.0f);
    if (combat->hitPause == 0)
        data->elapsed += dt;

    if (data->elapsed >= ability_base[ABILITY_STATE_CLAW].duration)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, 1);
        return;
    }

    if (data->elapsed < HITDELAY)
        return;
    data->accum += dt;

    if (data->accum > HITINTERVAL)
    {
        data->accum = 0;

        SolRay ray = {
            .start     = cmd->aimpos,
            .dir       = cmd->aimdir,
            .dist      = body->dims.x + MELEE_RANGE,
            .ignoreEnt = id,
            .mask      = 1,
        };
        SolRayResult results[128];
        int hits = Sol_RaycastD(world, ray, results, 128, 1.0f);
        sollog(hits);
        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];
            float dot = glms_vec3_dot(cmd->aimdir, glms_vec3_normalize(glms_vec3_sub(result.pos, cmd->aimpos)));
            if (dot < 0)
                continue;
            if (!Sol_Combat_TryHitGen(world, id, result.entId, combat->hitSession))
                continue;

            SolHit hit = {
                .damage     = ability_base[ABILITY_STATE_CLAW].damage,
                .buffMask   = ability_base[ABILITY_STATE_CLAW].buffMask,
                .effectMask = ability_base[ABILITY_STATE_CLAW].effectMask,
                .entA       = id,
                .entB       = result.entId,
                .pos        = result.pos,
                .vel        = cmd->aimdir,
            };
            if(Sol_Comp_Has(world, result.entId, ScBody3))
            {
                ScBody3 *body = Sol_Comp_Get(world, result.entId, ScBody3);
                body->vel.y += 50.0f;
            }
            Sol_Combat_Hit(world, result.entId, hit);

            if (combat->hitPauseDiminish < 4)
            {
                combat->hitPause = 1.0f;
                combat->hitPauseDiminish++;
            }

            body->vel.y = fmax(body->vel.y, 1.0f);
        }
    }
}

void Ability_Claw_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->accum            = HITINTERVAL;

    if (Sol_Comp_Has(world, id, ScCombat))
    {
        ScCombat *combat         = Sol_Comp_Get(world, id, ScCombat);
        combat->hitSession       = Sol_Combat_StartHitGen(world, id);
        combat->hitPause         = 0;
        combat->hitPauseDiminish = 0;
    }

    // Sol_Event_Add(world, (SolEvent){
    //                          .kind       = EVENTKIND_FX,
    //                          .as.fx.kind = FXKIND_SWORD_SWING,
    //                          .as.fx.pos  = Sol_Controller_GetShootPos(world, id, 1.0f),
    //                      });
}

void Ability_Claw_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    data->lastExited = world->tickTime;
    Sol_Anim_Stop(world, id, ANIM_LAYER_UPPER, 0);
}

bool Ability_Claw_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    return data->elapsed >= ability_base[ABILITY_STATE_CLAW].duration * 0.5f;
}

bool Ability_Claw_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];

    return slot != ability->activeSlot &&
           !(data->lastExited + ability_base[ABILITY_STATE_CLAW].cooldown > world->tickTime);
}

void Ability_Claw_Draw(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
}