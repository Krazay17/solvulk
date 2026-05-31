#include "sol_core.h"

#include "ability_i.h"

#define SHIELD_COOLDOWN 1.0

#define HITDURATION 0.5f
#define HITINTERVAL 0.1f

#define PULSEDAMAGE 20
#define PULSEKNOCKBACK 10.0f

void Shield_State_Update(World *world, int id, float dt)
{
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_SHIELD];
    if (!(Sol_Controller_GetActionState(world, id) & ACTION_ABILITY2))
        Sol_Ability_SetState(world, id, 0, false);

    data->elapsed += (float)dt;
    data->accum += (float)dt;
    Sol_Movement_SetSpeedMod(world, id, 0.5f);

    // Hit check a sphere every HITINTERVAL for HITDURATION to find hits and add to temp hit array to only hit unique
    // entities once
    if (data->elapsed < HITDURATION)
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
                if (world->flags[results[i].entId].flags & EFLAG_PROJECTILE)
                {
                    Sol_Physx_SetVel(world, results[i].entId,
                                     Sol_RedirectVel(Sol_Physx_GetVel(world, results[i].entId),
                                                     Sol_Controller_GetAimdir(world, id)));
                    Sol_Owner_SetOwner(world, results[i].entId, id);
                    continue;
                }
                Sol_Event_Add(world, (SolEvent){
                                         .kind             = EVENTKIND_HIT,
                                         .as.hit.entA      = id,
                                         .as.hit.entB      = results[i].entId,
                                         .as.hit.pos       = results[i].pos,
                                         .as.hit.damage    = PULSEDAMAGE,
                                         .as.hit.power     = PULSEKNOCKBACK,
                                         .as.hit.dir       = vecSub(results[i].pos, pos),
                                         .as.hit.buffKind  = {BUFFKIND_KNOCKBACK},
                                         .as.hit.buffcount = 1,
                                     });
            }
        }
}

void Shield_State_Enter(World *world, int id)
{
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_SHIELD];
    data->accum       = HITINTERVAL;
    Sol_Combat_AddFlags(world, id, COMBATFLAG_REFLECTING);
    CompCombat *combat = &world->combats[id];
    memset(combat->hitEnts, 0, sizeof(combat->hitEnts));

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
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_SHIELD];
    data->lastExited  = Sol_GetGameTime();
}

bool Shield_State_CanExit(World *world, int id, u32 next)
{
    return next != ABILITY_STATE_SHIELD;
}

bool Shield_State_CanEnter(World *world, int id, u32 last, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[next];
    return !(data->lastExited + SHIELD_COOLDOWN > Sol_GetGameTime());
}

void Shield_Rapid_Pulse(World *world, int id)
{
    vec3s pos = Sol_Xform_GetPos(world, id);

    Sol_Audio_PlayAt(SOL_AUDIO_WOONG, Sol_Controller_GetAimPos(world, id), 0.1f);
    SolRayResult results[256] = {0};
    int          hits         = Sol_SphereCast(world, (SolRay){.pos = pos}, 4.0f, results, 256);

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.kind = FXKIND_SHIELD_BURST,
                             .as.fx.pos  = pos,
                             .as.fx.entA = id,
                         });

    for (int i = 0; i < hits; i++)
    {
        SolRayResult result = results[i];
        if (!Sol_Owner_GetHostile(world, id, result.entId))
            continue;

        Sol_Event_Add(world, (SolEvent){
                                 .kind       = EVENTKIND_FX,
                                 .as.fx.pos  = result.pos,
                                 .as.fx.kind = FXKIND_SHIELD_HIT,
                                 .as.fx.entA = result.entId,
                             });
        if (world->flags[result.entId].flags & EFLAG_PROJECTILE)
        {
            Sol_Physx_SetVel(
                world, result.entId,
                Sol_RedirectVel(Sol_Physx_GetVel(world, result.entId), Sol_Controller_GetAimdir(world, id)));
            Sol_Owner_SetOwner(world, result.entId, id);
            continue;
        }
        Sol_Event_Add(world, (SolEvent){
                                 .kind             = EVENTKIND_HIT,
                                 .as.hit.entA      = id,
                                 .as.hit.entB      = result.entId,
                                 .as.hit.pos       = result.pos,
                                 .as.hit.damage    = 20,
                                 .as.hit.power     = 10.0f,
                                 .as.hit.dir       = vecSub(result.pos, pos),
                                 .as.hit.buffKind  = {BUFFKIND_KNOCKBACK},
                                 .as.hit.buffcount = 1,
                             });
    }
}