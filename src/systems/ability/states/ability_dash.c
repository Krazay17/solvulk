#include "world.h"
#include "estate.h"
#include "sol_core.h"
#include "sol_math.h"

#define DASH_VEL 20.0f
#define DASH_ALPHAMOD 1.5f

void Ability_Dash_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;

    if (data->elapsed >= data->conf.duration)
    {
        Sol_Ability_SetState(world, id, 0, ability->activeSlot, true);
        return;
    }
    float alpha = DASH_ALPHAMOD - (data->elapsed / data->conf.duration);

    if (Sol_Comp_Has(world, id, ScBody3))
    {
        ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
        body->vel     = glms_vec3_scale(data->as.dash.dir, alpha * DASH_VEL);
    }
    ScMove3 *move = Sol_Comp_Get(world, id, ScMove3);
    if(move)
    {
        // move->frictionMod = 0.0f;
    }
}

void Ability_Dash_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->conf             = Sol_Ability_GetSlotConf(ability, ability->activeSlot);

    vec3s flat_lookdir   = cmd->lookdir;
    flat_lookdir.y       = 0;
    flat_lookdir         = vecNorm(flat_lookdir);
    data->as.dash.dir    = flat_lookdir;
    data->as.dash.strafe = STRAFE_FWD;
    if (glms_vec3_norm2(cmd->wishdir) > 0)
    {
        data->as.dash.dir = cmd->wishdir;
    }

    data->as.dash.strafe =
        Sol_GetStrafedirYaw(data->as.dash.dir.x, data->as.dash.dir.z, Sol_Quat_ToYaw(world->xform.rot[id]));

    Sol_Buff_AddE(world, id, BUFFKIND_INVULN, id, 1.0f, 0.5f);
    data->cooldownRemaining = data->conf.cooldown;
}

void Ability_Dash_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
}

bool Ability_Dash_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    return data->elapsed > data->conf.duration * 0.8f;
}

bool Ability_Dash_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];

    return data->cooldownRemaining <= 0.0f;
}

extern const AbilityStateFunc ability_dash_state = {
    .update   = Ability_Dash_Update,
    .enter    = Ability_Dash_Enter,
    .exit     = Ability_Dash_Exit,
    .canExit  = Ability_Dash_CanExit,
    .canEnter = Ability_Dash_CanEnter,
};