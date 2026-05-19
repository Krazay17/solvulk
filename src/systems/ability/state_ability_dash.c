#include "sol_core.h"

#include "ability_i.h"

#define DASH_VEL 22.0f
#define DASH_ALPHAMOD 1.5f
#define DASH_DURATION 0.3f
#define DASH_COOLDOWN 1.0f

void ADash_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->state];
    float       *elapsed = &data->elapsed;
    *elapsed += dt;
    if (*elapsed >= DASH_DURATION)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE);
        return;
    }
    float alpha = DASH_ALPHAMOD - (*elapsed / DASH_DURATION);

    Sol_Physx_SetVel(world, id, glms_vec3_scale(data->dir, alpha * DASH_VEL));
}

void ADash_State_Enter(World *world, int id)
{
    Sol_Audio_Play(SOL_AUDIO_DASH);

    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ABILITY_STATE_DASH];

    data->dir = Sol_Vec3_FromYawPitch(Sol_GetYaw(world, id), 0);
    if (glms_vec3_norm(Sol_GetWishdir(world, id)) > 0 && vecDot(Sol_GetWishdir(world, id), WORLD_UP) < 0.99f)
        data->dir = Sol_GetWishdir(world, id);
    data->dir.y = 0;
    data->dir   = glms_vec3_normalize(data->dir);

    vec3s    rot  = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc = {.layerId = ANIM_LAYER_OVERRIDE};
    switch (Sol_GetStrafedir(data->dir.x, data->dir.z, rot.x, rot.z))
    {
    case STRAFE_FWD:
    case STRAFE_FWD_LEFT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_DASH_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_BWD:
    case STRAFE_BWD_LEFT:
    case STRAFE_BWD_RIGHT:
        desc.anim = ANIM_DASH_BWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_LEFT:
        desc.anim = ANIM_DASH_LEFT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_RIGHT:
        desc.anim = ANIM_DASH_RIGHT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    default:
        desc.anim = ANIM_DASH_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    }
}

void ADash_State_Exit(World *world, int id)
{
    Sol_Model_StopAnim(world, id, ANIM_LAYER_OVERRIDE, 0.15f);
}

bool ADash_State_CanEnter(World *world, int id, int last)
{
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_DASH];
    return !(data->lastEntered + DASH_COOLDOWN > (float)Sol_GetState()->gameTime);
}

bool ADash_State_CanExit(World *world, int id, int next)
{
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_DASH];
    return data->elapsed >= DASH_DURATION * 0.9f;
}
