/*
 * File: ability_state_claw.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "s_ability.h"
#include "world.h"
#include "sol_math.h"

#define HITDELAY 0.2f
#define HITINTERVAL 0.1f
#define MELEE_RANGE 1.0f

void Claw_State_Update(World *world, int id, float dt)
{
    ScAbility        *ability = Sol_Comp_Get(world, id, ScAbility);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    ScCombat         *combat  = Sol_Comp_Get(world, id, ScCombat);
    ScBody3          *body    = Sol_Comp_Get(world, id, ScBody3);
    ScAnim           *anim    = Sol_Comp_Get(world, id, ScAnim);

    combat->hitPause = fmaxf(0, combat->hitPause - dt * 5.0f);
    if (combat->hitPause > 0)
        Sol_Anim_SetSpeed(world, id, ANIM_LAYER_UPPER, -0.001f);
    else
    {
        Sol_Anim_SetSpeed(world, id, ANIM_LAYER_UPPER, combat->baseAnimRate);
        data->elapsed += dt;
    }

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
        ScCmd *cmd  = Sol_Comp_Get(world, id, ScCmd);
        data->accum = 0;

        SolRay ray = {
            .start     = cmd->aimpos,
            .dir       = cmd->aimdir,
            .dist      = MELEE_RANGE,
            .ignoreEnt = id,
        };
        SolRayResult results[128];
        int          hits = Sol_RaycastD(world, ray, results, 128, 0.5f);

        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];

            float dot = glms_vec3_dot(cmd->aimdir, glms_vec3_normalize(glms_vec3_sub(result.pos, cmd->aimpos)));
            if (dot < 0)
                continue;
            // if (combat->hitEnts[result.entId])
            //     continue;
            // combat->hitEnts[result.entId] = true;
            // if (!Sol_Combat_TryHitGen(world, id, result.entId, data->hitSessionGen))
            //     continue;

            SolHit hit = {
                .damage     = ability_base[ABILITY_STATE_CLAW].damage,
                .buffMask   = ability_base[ABILITY_STATE_CLAW].buffMask,
                .effectMask = ability_base[ABILITY_STATE_CLAW].effectMask,
                .entA       = id,
                .entB       = result.entId,
                .pos        = result.pos,
                .vel        = cmd->aimdir,
            };
            // Sol_Combat_ApplyHit(world, result.entId, hit);

            if (combat->hitPauseDiminish < 4)
            {
                combat->hitPause = 1.0f;
                combat->hitPauseDiminish++;
            }

            body->vel.y = fmax(body->vel.y, 1.0f);
        }
    }
}

void Claw_State_Enter(World *world, int id)
{
    ScAbility        *ability = Sol_Comp_Get(world, id, ScAbility);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    ScCombat         *combat  = Sol_Comp_Get(world, id, ScCombat);

    data->accum = HITINTERVAL;
    // data->hitSessionGen = Sol_Combat_StartHitGen(world, id);
    // Sol_Combat_ClearHits(world, id);
    float animRate           = 1.0f;
    combat->baseAnimRate     = animRate;
    combat->hitPause         = 0;
    combat->hitPauseDiminish = 0;

    // Sol_Event_Add(world, (SolEvent){
    //                          .kind       = EVENTKIND_FX,
    //                          .as.fx.kind = FXKIND_SWORD_SWING,
    //                          .as.fx.pos  = Sol_Controller_GetShootPos(world, id, 1.0f),
    //                      });
}

void Claw_State_Exit(World *world, int id)
{
    ScAbility        *ability = Sol_Comp_Get(world, id, ScAbility);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];

    data->lastExited = world->tickTime;
    Sol_Anim_Stop(world, id, ANIM_LAYER_UPPER, 0);
}

bool Claw_State_CanExit(World *world, int id, u32 next)
{
    ScAbility        *ability = Sol_Comp_Get(world, id, ScAbility);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];

    return data->elapsed >= ability_base[ABILITY_STATE_CLAW].duration * 0.5f;
}

bool Claw_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    ScAbility        *ability = Sol_Comp_Get(world, id, ScAbility);
    AbilityStateData *data    = &ability->stateData[slot];
    
    return slot != ability->activeSlot &&
           !(data->lastExited + ability_base[ABILITY_STATE_CLAW].cooldown > world->tickTime);
}
