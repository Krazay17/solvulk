#include "sol_core.h"

#include "ability_i.h"

void Shield_State_Update(World *world, int id, float dt)
{
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_SHIELD];

    if (!(Sol_Controller_GetActionState(world, id) & ACTION_ABILITY2))
        Sol_Ability_SetState(world, id, 0);

    Sol_Movement_SetSpeedMod(world, id, 0.5f);
    vec3s pos = Sol_Xform_GetPos(world, id);

    data->accum += (float)dt;
    if (data->accum > 0.5f)
    {
        data->accum = 0;
        SolRayResult results[256];
        int          hits = Sol_SphereCast(world, (SolRay){.pos = pos}, 5.0f, results, 256);

        Sol_Event_Add(world, (SolEvent){
                                 .kind       = EVENTKIND_FX,
                                 .as.fx.kind = FXKIND_SHIELD_BURST,
                                 .as.fx.pos  = pos,
                                 .sourceId   = id,
                             });

        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];
            if (!Sol_Vital_GetHostile(world, id, result.entId))
                continue;

            Sol_Event_Add(world, (SolEvent){
                                     .kind       = EVENTKIND_FX,
                                     .as.fx.pos  = result.pos,
                                     .as.fx.kind = FXKIND_SHIELD_HIT,
                                     .targetId   = result.entId,
                                 });
            if (world->flags[result.entId].flags & EFLAG_PROJECTILE)
            {
                Sol_Physx_SetVel(world, result.entId, vecSca(Sol_Physx_GetVel(world, result.entId), -1.0f));
                Sol_Owner_SetOwner(world, result.entId, id);
                continue;
            }
            Sol_Event_Add(world, (SolEvent){
                                     .kind             = EVENTKIND_HIT,
                                     .as.hit.pos       = result.pos,
                                     .as.hit.target    = result.entId,
                                     .as.hit.damage    = 20,
                                     .as.hit.power     = 25.0f,
                                     .as.hit.dir       = vecSub(result.pos, pos),
                                     .as.hit.buffs     = {{.kind = BUFFKIND_KNOCKBACK, .duration = 0.2f}},
                                     .as.hit.buffcount = 1,
                                 });
        }
    }
}

void Shield_State_Enter(World *world, int id)
{
    Sol_Combat_AddFlags(world, id, COMBATFLAG_REFLECTING);
}

void Shield_State_Exit(World *world, int id)
{
    Sol_Combat_RemoveFlags(world, id, COMBATFLAG_REFLECTING);
}

bool Shield_State_CanEnter(World *world, int id, int last)
{
    CompAbility *ability = &world->abilities[id];
    return !(ability->state == ABILITY_STATE_SHIELD);
}

bool Shield_State_CanExit(World *world, int id, int next)
{
    return true;
}
