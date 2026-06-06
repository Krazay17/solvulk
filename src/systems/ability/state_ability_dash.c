#include "sol_core.h"

#include "ability_i.h"

#define DASH_VEL 20.0f
#define DASH_ALPHAMOD 1.5f
#define DASH_DURATION 0.25f
#define DASH_COOLDOWN 1.0f

void ADash_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    float       *elapsed = &data->elapsed;
    *elapsed += dt;
    if (*elapsed >= DASH_DURATION)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, false);
        return;
    }
    float alpha = DASH_ALPHAMOD - (*elapsed / DASH_DURATION);

    Sol_Physx_SetVel(world, id, glms_vec3_scale(data->dir, alpha * DASH_VEL));
}

void ADash_State_Enter(World *world, int id)
{
    Sol_Audio_PlayAt(SOL_AUDIO_DASH, Sol_Controller_GetAimPos(world, id), 0.0f);
    Sol_Buff_Add(world, id, BUFFKIND_INVULN, id, 0.2f, 0);

    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];

    data->dir = Sol_Vec3_FromYawPitch(Sol_GetYaw(world, id), 0);
    if (glms_vec3_norm(Sol_GetWishdir(world, id)) > 0 && vecDot(Sol_GetWishdir(world, id), WORLD_UP) < 0.99f)
        data->dir = Sol_GetWishdir(world, id);
    data->dir.y = 0;
    data->dir   = glms_vec3_normalize(data->dir);

    vec3s    rot  = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc = {.layerId = ANIM_LAYER_OVERRIDE, .force = true, .oneShot = true, .seek = 0.1f, .blendIn = 0.1f};

    switch (Sol_GetStrafedir(data->dir.x, data->dir.z, rot.x, rot.z))
    {
    case STRAFE_FWD:
    case STRAFE_FWD_LEFT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_DASH_FWD;
        break;
    case STRAFE_BWD:
    case STRAFE_BWD_LEFT:
    case STRAFE_BWD_RIGHT:
        desc.anim = ANIM_DASH_BWD;
        break;
    case STRAFE_LEFT:
        desc.anim = ANIM_DASH_LEFT;
        break;
    case STRAFE_RIGHT:
        desc.anim = ANIM_DASH_RIGHT;
        break;
    default:
        desc.anim = ANIM_DASH_FWD;
        break;
    }
    Sol_Model_PlayAnim(world, id, desc);
    // Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_ANIM, .as.anim.animId = desc.anim, .as.anim.entId = id,
    // .as.anim.layerId = desc.layerId});
}

void ADash_State_Exit(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_OVERRIDE});
}

bool ADash_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    return data->elapsed >= DASH_DURATION * 0.9f;
}

bool ADash_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastEntered + ability_config[ABILITY_STATE_DASH].cooldown > Sol_GetGameTime());
}
