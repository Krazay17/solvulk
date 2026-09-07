#include "ability/s_ability.h"
#include "world.h"

void Ability_Fireball_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
    switch (data->stage)
    {
    case 0:
        if (!data->held)
            data->stage++;
        break;
    case 1:
    break;
    }
}

void Ability_Fireball_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->duration         = ability_base[ABILITY_STATE_FIREBALL].duration;
    data->cooldown         = ability_base[ABILITY_STATE_FIREBALL].cooldown;
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
    return data->lastExited + data->cooldown > world->tickTime;
}

void Ability_Fireball_Draw(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    if (data->stage > 0)
        return;
    float scale = data->power * 2.0f + 0.5f;
    // vec3s pos   = Sol_Model_GetBoneXform(world, id, "hand.L").pos;
    // pos         = vecAdd(pos, vecSca(Sol_Controller_Get(world, id)->aimdir, scale));
    // pos         = vecAdd(pos, vecSca(WORLD_UP, scale));

    // SphereSSBO *push = Sol_Render_GetNext_Fireball();
    // push->pos        = (vec4s){pos.x, pos.y, pos.z, scale};
    // push->color      = (vec4s){1, 0, 0, 0.8f};
}
