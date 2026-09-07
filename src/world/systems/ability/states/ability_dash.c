#include "ability/s_ability.h"
#include "world.h"
#include "sol_math.h"

#define DASH_VEL 20.0f
#define DASH_ALPHAMOD 1.5f

void Ability_Dash_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;

    if (data->elapsed >= data->duration)
    {
        Sol_Ability_SetState(world, id, 0, ability->activeSlot, true);
        return;
    }
    float alpha = DASH_ALPHAMOD - (data->elapsed / data->duration);

    if (Sol_Comp_Has(world, id, ScBody3))
    {
        ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
        body->vel     = glms_vec3_scale(data->as.dash.dir, alpha * DASH_VEL);
    }
}

void Ability_Dash_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->duration         = ability_base[ABILITY_STATE_DASH].duration;
    data->cooldown         = ability_base[ABILITY_STATE_DASH].cooldown;

    vec3s flat_lookdir     = cmd->lookdir;
    flat_lookdir.y         = 0;
    flat_lookdir           = vecNorm(flat_lookdir);
    data->as.dash.dir = flat_lookdir;
    data->as.dash.strafe   = STRAFE_FWD;
    if (glms_vec3_norm2(cmd->wishdir) > 0)
    {
        data->as.dash.dir = cmd->wishdir;
    }

    data->as.dash.strafe =
        Sol_GetStrafedirYaw(data->as.dash.dir.x, data->as.dash.dir.z, Sol_Quat_ToYaw(world->xform.rot[id]));
}

void Ability_Dash_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
}

bool Ability_Dash_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    return data->elapsed > data->duration * 0.8f;
}

bool Ability_Dash_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];

    return data->lastExited + data->cooldown < world->tickTime;
}
