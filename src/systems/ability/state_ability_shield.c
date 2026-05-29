#include "sol_core.h"

#include "ability_i.h"

#define SHIELD_COOLDOWN 1.0f
#define PULSERATE 0.5f

void Shield_State_Update(World *world, int id, float dt)
{
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_SHIELD];

    if (!(Sol_Controller_GetActionState(world, id) & ACTION_ABILITY2))
        Sol_Ability_SetState(world, id, 0, false);

    Sol_Movement_SetSpeedMod(world, id, 0.5f);
    vec3s pos = Sol_Xform_GetPos(world, id);

    data->accum += (float)dt;
    if (data->accum > PULSERATE)
    {
        Sol_Audio_PlayAt(SOL_AUDIO_WOONG, Sol_Controller_GetAimPos(world, id), 0.1f);
        data->accum               = 0;
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
}

void Shield_State_Enter(World *world, int id)
{
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_SHIELD];
    data->accum       = PULSERATE;
    Sol_Combat_AddFlags(world, id, COMBATFLAG_REFLECTING);
}

void Shield_State_Exit(World *world, int id)
{
    Sol_Combat_RemoveFlags(world, id, COMBATFLAG_REFLECTING);
}

bool Shield_State_CanExit(World *world, int id, u32 next)
{
    return next != ABILITY_STATE_SHIELD;
}

bool Shield_State_CanEnter(World *world, int id, u32 last, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[next];
    return !(data->lastEntered + SHIELD_COOLDOWN > (float)Sol_GetGameTime());
}
