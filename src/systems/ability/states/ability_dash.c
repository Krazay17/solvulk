#include "world.h"
#include "estate.h"
#include "sol_core.h"
#include "sol_math.h"

#define DASH_VEL 20.0f
#define DASH_ALPHAMOD 1.5f

static void Spell(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->elapsed += dt;

    if (data->elapsed >= data->conf.duration)
    {
        Sol_Ability_SetState(world, id, 0, slot, true);
        return;
    }
    float alpha = DASH_ALPHAMOD - (data->elapsed / data->conf.duration);

    if (Sol_Comp_Has(world, id, ScBody3))
    {
        ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
        body->vel     = glms_vec3_scale(data->as.dash.dir, alpha * DASH_VEL);
    }
}

void Ability_Dash_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->conf             = Sol_Ability_GetSlotConf(ability, slot);

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

    Sol_Buff_AddE(world, id, BUFFKIND_INVULN, id, 1.0f, data->conf.duration);
}

void Ability_Dash_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->cooldownRemaining = data->conf.cooldown;
}

bool Ability_Dash_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return data->elapsed > data->conf.duration * 0.8f;
}

bool Ability_Dash_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];

    return !(data->cooldownRemaining > 0.0f);
}

const AbilityStateFunc ability_dash_state = {
    .update   = Spell,
    .enter    = Ability_Dash_Enter,
    .exit     = Ability_Dash_Exit,
    .canExit  = Ability_Dash_CanExit,
    .canEnter = Ability_Dash_CanEnter,
};