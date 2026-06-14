#include "sol_core.h"

#include "ability_i.h"

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
            data->accum               = 0;
            vec3s        pos          = Sol_Xform_GetPos(world, id);
            SolRayResult results[256] = {0};
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
    memset(combat->hitEnts, 0, sizeof(combat->hitEnts));
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
    data->lastExited     = Sol_GetGameTime();
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
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > Sol_GetGameTime());
}