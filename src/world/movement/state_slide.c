#include "movement_i.h"
#include "core/sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

#define BOOST_CD 2.5f

static bool LeaveState(World *world, int id)
{
    if (Sol_Physx_GetSpeed(world, id) < 5.5f)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (!(Sol_GetActions(world, id) & ACTION_CROUCH))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (Sol_GetActions(world, id) & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    // if (Sol_Movement_GetAirtime(world, id) > 0.1f)
    //     if (Sol_Movement_SetState(world, id, MOVE_FALL))
    //         return true;
    return false;
}

void Slide_State_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;
    float          fdt    = (float)dt;
    CompMovement  *move   = &world->movements[id];
    MoveStateData *data   = &move->stateData[move->state];
    vec3s          vel    = Sol_Physx_GetVel(world, id);
    vec3s          rot    = Sol_RotFromQuat(world->xforms[id].quat);
    vec3s          latvel = vel;
    latvel.y              = 0;
    latvel                = vecNorm(latvel);
    data->as.slide.boost  = fmaxf(data->as.slide.boost - fdt, 0.0f);

    if (move->groundDot > 0.01f && move->groundDot < 0.99f)
    {
        Sol_Physx_AddVel(world, id, vecSca(vecNorm(GroundSlope(world, id)), 12.0f * fdt));
    }

    AnimDesc desc = {.layerId = ANIM_LAYER_BASE};
    switch (Sol_GetStrafedir(vel.x, vel.z, rot.x, rot.z))
    {
    case STRAFE_FWD:
        desc.anim = ANIM_SLIDE_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_BWD:
    case STRAFE_BWD_RIGHT:
    case STRAFE_BWD_LEFT:
        desc.anim = ANIM_SLIDE_BWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_LEFT:
    case STRAFE_FWD_LEFT:
        desc.anim = ANIM_SLIDE_LEFT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_RIGHT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_SLIDE_RIGHT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    default:
        desc.anim = ANIM_SLIDE_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    }

    if (move->groundtime > 0)
    {
        const MoveStateForce *forces   = &MOVE_STATE_FORCES[move->kind][move->state];
        float                 speedDif = (Sol_Physx_GetSpeed(world, id) / forces->speed) * 0.5f;
        speedDif                       = fminf(speedDif, 3.0f);
        Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_BASE, speedDif);
    }
    else
        Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_BASE, 0.5f);
}

void Slide_State_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->state];
    move->targetHeight  = move->baseHeight * 0.65f;
    data->lastEntered   = solState.gameTime;
    if (move->groundtime > 0)
    {
        data->as.slide.boost = fminf(data->as.slide.boost + (solState.gameTime - data->lastExited), BOOST_CD);
        Sol_Physx_Impulse(world, id,
                          vecSca(vecNorm(ProjectOntoGround(world, id, Sol_Physx_GetVelDir(world, id))),
                                 Sol_Math_MapRange(0.0f, 400.0f, 0.0f, BOOST_CD, data->as.slide.boost)));
        data->as.slide.boost /= 2.0f;
    }
}

void Slide_State_Exit(World *world, int id)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_SLIDE];
    data->lastExited    = solState.gameTime;
    move->targetHeight  = move->baseHeight;
}

bool Slide_State_CanExit(World *world, int id, u32 nextState)
{
    return true;
}

bool Slide_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    return Sol_Physx_GetSpeed(world, id) > 5.5f;
}
