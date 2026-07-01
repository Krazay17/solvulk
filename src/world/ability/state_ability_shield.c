#include "ability_i.h"
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
#include "render/render.h"

#define HITINTERVAL 0.1f

void Shield_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];

    data->elapsed += (float)dt;
    data->accum += (float)dt;
    Sol_Movement_SetSpeedMod(world, id, 0.5f);

    // aoe scan over time, hit ents once
    if (data->elapsed < data->duration)
    {
        if (data->accum > HITINTERVAL)
        {
            data->accum      = 0;
            vec3s        pos = Sol_Xform_GetPos(world, id);
            SolRayResult results[256];
            int          hits = Sol_SphereCast(world, (SolRay){.pos = pos, .ignoreEnt = id}, 4.0f, results, 256);
            for (int i = 0; i < hits; i++)
            {
                CompCombat *combat = &world->combats[id];
                if (combat->hitEnts[results[i].entId])
                    continue;
                combat->hitEnts[results[i].entId] = true;
                Sol_Event_Add(world, (SolEvent){
                                         .kind              = EVENTKIND_HIT,
                                         .as.hit.entA       = id,
                                         .as.hit.entB       = results[i].entId,
                                         .as.hit.pos        = results[i].pos,
                                         .as.hit.kind       = HITKIND_SHIELD_PULSE,
                                         .as.hit.vel        = vecSub(results[i].pos, pos),
                                         .as.hit.effectMask = data->effects,
                                         .as.hit.buffMask   = data->buffs,
                                         .as.hit.damage     = data->damage,
                                         .as.hit.fxKind     = FXKIND_SHIELD_HIT,
                                     });
            }
        }
    }
    else
    {
        Sol_Ability_SetState(world, id, 0, 0, false);
        return;
    }
}

void Shield_State_Enter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->accum          = HITINTERVAL;
    Sol_Combat_AddFlags(world, id, COMBATFLAG_REFLECTING);
    CompCombat *combat = &world->combats[id];
    Sol_Combat_ClearHits(world, id);
    Sol_Buff_AddEx(world, id, id, BUFFKIND_INVULN, data->duration, 0);
    vec3s pos = Sol_Xform_GetPos(world, id);
    Sol_Audio_PlayAt(SOL_AUDIO_WOONG, Sol_Controller_GetAimPos(world, id), 1.0f, 0.1f, 0);
    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.kind = FXKIND_SHIELD_BURST,
                             .as.fx.pos  = pos,
                             .as.fx.entA = id,
                         });
}

void Shield_State_Exit(World *world, int id)
{
    Sol_Combat_RemoveFlags(world, id, COMBATFLAG_REFLECTING);
    Sol_Buff_Remove(world, id, BUFFKIND_INVULN);
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited     = solState.gameTime;
}

bool Shield_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    return true;
    return data->elapsed >= data->duration;
}

bool Shield_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > solState.gameTime);
}

void Shield_State_Draw(World *world, int id, double dt, double time)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    CompXform   *xform   = &world->xforms[id];
    
    SphereSSBO  *o       = Sol_Render_GetNext_Sphere(true);
    o->pos               = (vec4s){xform->drawPos.x, xform->drawPos.y, xform->drawPos.z, 1.0f};
    o->color             = (vec4s){0.25f, 0.1f, 0.5f, 0.25f};
}