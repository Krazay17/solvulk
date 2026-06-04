#include "sol_core.h"

#include "ability_i.h"

#define SHIELD_COOLDOWN 1.0

#define HITDURATION 0.5f
#define HITINTERVAL 0.1f

#define PULSEDAMAGE 20

void Shield_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data = &ability->stateData[ability->activeSlot];

    data->elapsed += (float)dt;
    data->accum += (float)dt;
    Sol_Movement_SetSpeedMod(world, id, 0.5f);

    // aoe scan over time, hit ents once
    if (data->elapsed < HITDURATION)
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
                if (!Sol_Owner_GetHostile(world, id, results[i].entId))
                    continue;
                if (combat->hitEnts[results[i].entId])
                    continue;
                combat->hitEnts[results[i].entId] = true;

                Sol_Event_Add(world, (SolEvent){
                                         .kind       = EVENTKIND_FX,
                                         .as.fx.pos  = results[i].pos,
                                         .as.fx.kind = FXKIND_SHIELD_HIT,
                                         .as.fx.entA = results[i].entId,
                                     });
                if (world->masks[results[i].entId] & HAS_PROJECTILE)
                {
                    Sol_Physx_SetRedirectVel(world, results[i].entId, Sol_Controller_GetAimdir(world, id));
                    Sol_Owner_Add(world, results[i].entId, id);
                    continue;
                }
                Sol_Event_Add(world, (SolEvent){
                                         .kind        = EVENTKIND_HIT,
                                         .as.hit.entA = id,
                                         .as.hit.entB = results[i].entId,
                                         .as.hit.pos  = results[i].pos,
                                         .as.hit.kind = HITKIND_SHIELD_PULSE,
                                         .as.hit.vel  = vecSub(results[i].pos, pos),
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
    AbilityData *data = &ability->stateData[ability->activeSlot];
    data->accum       = HITINTERVAL;
    Sol_Combat_AddFlags(world, id, COMBATFLAG_REFLECTING);
    CompCombat *combat = &world->combats[id];
    memset(combat->hitEnts, 0, sizeof(combat->hitEnts));
    Sol_Buff_Add(world, id, BUFFKIND_INVULN, id, 0.5f, 0);

    vec3s pos = Sol_Xform_GetPos(world, id);
    Sol_Audio_PlayAt(SOL_AUDIO_WOONG, Sol_Controller_GetAimPos(world, id), 0.1f);
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
    CompAbility *ability = &world->abilities[id];
    AbilityData *data = &ability->stateData[ability->activeSlot];
    data->lastExited  = Sol_GetGameTime();
}

bool Shield_State_CanExit(World *world, int id, u32 next)
{
        CompAbility *ability = &world->abilities[id];
    AbilityData *data = &ability->stateData[ability->activeSlot];

    return next != ABILITY_STATE_SHIELD && data->elapsed >= HITDURATION;
}

bool Shield_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return !(data->lastExited + SHIELD_COOLDOWN > Sol_GetGameTime());
}