#include "ability/s_ability.h"
#include "world.h"
#include "sol_math.h"

#include "render/render.h"
#include "prefabs.h"

#define MIN_POWER 0.2f
#define MAX_POWER 1.5f

static vec3s GetProjectilePos(World *world, int id, ScCmd *cmd, float power)
{
    vec3s pos = Sol_Model_GetBoneXform(world, id, "hand.L").pos;
    pos       = vecAdd(pos, vecSca(cmd->aimdir, (power - (MIN_POWER + 0.2f))));
    pos       = vecAdd(pos, vecSca(WORLD_UP, (power - (MIN_POWER + 0.6f))));
    return pos;
}

void Ability_Fireball_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
    switch (data->stage)
    {
    case 0:
        data->power = Sol_Math_Lerp_Clamped(MIN_POWER, MAX_POWER, data->elapsed / data->duration);

        if (!data->held)
            data->stage++;
        break;
    case 1:
        data->stage++;
        vec3s pos = GetProjectilePos(world, id, cmd, data->power);
        vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));
        Sol_Prefab_Fireball(world, id, pos, dir, 25.0f, data->power);
    case 2:
        data->recover += dt;
        if (data->recover > data->recoverDuration)
            Sol_Ability_SetState(world, id, 0, ability->activeSlot, true);
        break;
    }
}

void Ability_Fireball_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->duration         = ability_base[ABILITY_STATE_FIREBALL].duration;
    data->cooldown         = ability_base[ABILITY_STATE_FIREBALL].cooldown;
    data->recoverDuration  = ability_base[ABILITY_STATE_FIREBALL].recoverDuration;
}

void Ability_Fireball_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
}

bool Ability_Fireball_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    return data->elapsed > data->duration * 0.8f;
}

bool Ability_Fireball_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return data->lastExited + data->cooldown < world->tickTime;
}

void Ability_Fireball_Draw(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    if (data->stage > 0)
        return;

    SphereSSBO *push = Sol_Render_GetNextSphere(SPHEREKIND_FIREBALL);

    vec3s pos   = GetProjectilePos(world, id, cmd, data->power);
    push->pos   = (vec4s){pos.x, pos.y, pos.z, data->power};
    push->color = (vec4s){1, 0, 0, 0.8f};
}
