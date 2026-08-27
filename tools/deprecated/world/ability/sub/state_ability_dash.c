#include "ability/si_ability.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "audio.h"

#include "xform/s_xform.h"
#include "controller/s_controller.h"
#include "combat/s_combat.h"
#include "model/s_model.h"
#include "event/s_event.h"
#include "physx/s_body.h"
#include "movement/s_movement.h"
#include "buff/s_buff.h"
#include "item/s_item.h"
#include "inventory/s_inventory.h"

#define DASH_VEL 20.0f
#define DASH_ALPHAMOD 1.5f
#define HITINTERVAL 0.05f
#define HITRADIUS 1.7f

void ADash_State_Update(World *world, int id, float dt)
{
    CompAbility      *ability  = Sol_Ability_Get(world, id);
    AbilityStateData *data     = &ability->stateData[ability->activeSlot];
    AbilityConfig     augments = (AbilityConfig){0};
    SolItem          *item     = Sol_Inventory_GetItemAtSlot(world, id, ability->activeSlot);
    if (item)
        augments = item->ability;
    data->elapsed += dt;
    data->accum += dt;

    if (data->elapsed >= ability_base[ABILITY_STATE_DASH].duration + augments.duration)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, false);
        return;
    }

    float alpha = DASH_ALPHAMOD - (data->elapsed / ability_base[ABILITY_STATE_DASH].duration);
    Sol_Physx_SetVel(world, id, glms_vec3_scale(data->as.dash.enterDir, alpha * DASH_VEL));

    if (!(data->doesHit))
        return;
    if (data->accum > HITINTERVAL)
    {
        data->accum = 0;

        vec3s pos = Sol_Xform_GetPos(world, id);
        pos       = glms_vec3_add(pos, glms_vec3_scale(Sol_Physx_GetVelDir(world, id), 1.0f));
        SolRayResult results[256];
        int          hits = Sol_SphereCast(world, (SolRay){.pos = pos, .ignoreEnt = id}, HITRADIUS, results, 256);

        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];
            CompCombat  *combat = Sol_Combat_Get(world, id);
            // if (!Sol_Owner_GetHostile(world, id, results[i].entId))
            //     continue;
            // if (combat->hitEnts[results[i].entId])
            //     continue;
            // combat->hitEnts[results[i].entId] = true;

            if (!Sol_Combat_TryHitGen(world, id, result.entId, data->hitSessionGen))
                continue;

            Sol_Combat_ApplyHit(world, results[i].entId,
                                (SolHit){
                                    .entA       = id,
                                    .entB       = results[i].entId,
                                    .pos        = result.pos,
                                    .damage     = ability_base[ABILITY_STATE_DASH].damage + augments.damage,
                                    .effectMask = ability_base[ABILITY_STATE_DASH].effectMask | augments.effectMask,
                                    .buffMask   = ability_base[ABILITY_STATE_DASH].buffMask | augments.buffMask,
                                    .vel        = vecSub(result.pos, pos),
                                });
            Sol_Event_Add(world,
                          (SolEvent){.kind = EVENTKIND_FX, .as.fx.kind = FXKIND_SPINHIT, .as.fx.pos = result.pos});
        }
    }
}

void ADash_State_Enter(World *world, int id)
{
    CompAbility      *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    AbilityConfig     cfg     = {0};
    SolItem          *item    = Sol_Inventory_GetItemAtSlot(world, id, ability->activeSlot);
    if (item)
        cfg = item->ability;
    data->duration = ability_base[ABILITY_STATE_DASH].duration + cfg.duration;

    Sol_Buff_AddEx(world, id, id, BUFFKIND_INVULN, ability_base[ABILITY_STATE_DASH].duration * 0.5f, 0);
    Sol_Combat_ClearHits(world, id);
    data->hitSessionGen = Sol_Combat_StartHitGen(world, id);

    vec3s dir = Sol_Vec3_FromYawPitch(Sol_Controller_Get(world, id)->yaw, 0);
    if (glms_vec3_norm(Sol_Controller_Get(world, id)->wishdir) > 0 &&
        vecDot(Sol_Controller_Get(world, id)->wishdir, WORLD_UP) < 0.99f)
        dir = Sol_Controller_Get(world, id)->wishdir;
    dir.y                  = 0;
    dir                    = glms_vec3_normalize(dir);
    data->as.dash.enterDir = dir;

    vec3s rot            = Sol_RotFromQuat(world->xforms[id].quat);
    data->as.dash.strafe = Sol_GetStrafedir(dir.x, dir.z, rot.x, rot.z);

    Sol_Event_Add(world, (SolEvent){.kind          = EVENTKIND_SOUND,
                                    .as.sound.kind = SOL_AUDIO_DASH,
                                    .as.sound.pos  = Sol_Xform_GetPos(world, id)});
}

void ADash_State_Exit(World *world, int id)
{
    CompAbility      *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited          = solState.gameTime;
}

bool ADash_State_CanExit(World *world, int id, u32 next)
{
    CompAbility      *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    return data->elapsed >= ability_base[ABILITY_STATE_DASH].duration * 0.5f;
}

bool ADash_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility      *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot &&
           !(data->lastExited + ability_base[ABILITY_STATE_DASH].cooldown > solState.gameTime);
}
