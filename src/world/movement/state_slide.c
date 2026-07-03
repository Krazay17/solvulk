#include "movement_i.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

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
    // if (Sol_Physx_GetAirtime(world, id) > 0.1f)
    //     if (Sol_Movement_SetState(world, id, MOVE_FALL))
    //         return true;
    return false;
}

void Slide_State_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;

    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->state];

    float    x    = Sol_Controller_GetWishdir(world, id).x;
    float    z    = Sol_Controller_GetWishdir(world, id).z;
    vec3s    rot  = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc = {.layerId = ANIM_LAYER_BASE};
    switch (Sol_GetStrafedir(x, z, rot.x, rot.z))
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

    // const MoveStateForce *forces   = &MOVE_STATE_FORCES[move->configId][move->state];
    // float                 speedDif = Sol_Physx_GetSpeed(world, id) / forces->speed;
    // Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_BASE, speedDif);

    Sol_Physx_Impulse(world, id, vecSca(GroundSlope(world, id), 10.0f));
}

void Slide_State_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->state];
    move->targetHeight  = move->baseHeight * 0.65f;

    vec3s wishdir  = Sol_Controller_GetWishdir(world, id);
    vec3s slopeDir = ProjectOntoGround(world, id, wishdir);
    // float dot      = glms_vec3_dot(wishdir, Sol_Physx_GetGround(world, id));
    // vec3s slopeDir = glms_vec3_sub(wishdir, glms_vec3_scale(Sol_Physx_GetGround(world, id), dot));

    Sol_Physx_Impulse(world, id, vecSca(slopeDir, 250.0f));
}

void Slide_State_Exit(World *world, int id)
{
    CompMovement *move = &world->movements[id];
    move->targetHeight = move->baseHeight;
}

bool Slide_State_CanExit(World *world, int id, u32 nextState)
{
    return true;
}

bool Slide_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    //sollog(Sol_Physx_Get_Ground_Norm(world, id));
    return Sol_Physx_GetSpeed(world, id) > 5.5f;
}
