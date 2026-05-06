#include "sol_core.h"

#define DASH_VEL 26.0f
#define DASH_ALPHAMOD 1.2f
#define DASH_DURATION 0.3f
#define DASH_COOLDOWN 1.0f

void ADash_State_Update(World *world, int id, float dt)
{
    CompBody    *body    = &world->bodies[id];
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
    vec3s vel   = glms_vec3_scale(data->dir, alpha * DASH_VEL);
    body->vel   = vel;
}

void ADash_State_Enter(World *world, int id)
{
    CompAbility    *ability    = &world->abilities[id];
    CompController *controller = &world->controllers[id];
    AbilityData    *data       = &ability->stateData[ABILITY_STATE_DASH];

    data->dir = Sol_Vec3_FromYawPitch(controller->yaw, 0);
    if (glms_vec3_norm(controller->wishdir) > 0 && vecDot(controller->wishdir, WORLD_UP) < 0.99f)
        data->dir = controller->wishdir;
    data->dir.y = 0;
    data->dir   = glms_vec3_normalize(data->dir);

    AnimDesc desc = {.layerId = ANIM_LAYER_OVERRIDE};
    switch (Get_StrafeDir(data->dir.x, data->dir.z, controller->lookdir.x, controller->lookdir.z))
    {
    case STRAFE_FWD:
        desc.anim = ANIM_DASH_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_BWD:
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
    }
}

void ADash_State_Exit(World *world, int id)
{
    Sol_Model_StopAnim(world, id, ANIM_LAYER_OVERRIDE, 0.15f);
}

bool ADash_State_CanEnter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ABILITY_STATE_DASH];
    if (data->lastEntered + DASH_COOLDOWN > (float)Sol_GetState()->gameTime)
        return false;

    return true;
}

bool ADash_State_CanExit(World *world, int id, AbilityState next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ABILITY_STATE_DASH];

    if (data->elapsed > DASH_DURATION)
        return true;
    else
        return false;
}
