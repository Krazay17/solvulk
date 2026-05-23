#include "sol_core.h"

#include "movement_i.h"

static bool CanSlide(World *world, int id)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->moveState];

    return false;
}

static bool LeaveState(World *world, int id)
{
    if (Sol_Movement_SetState(world, id, MOVE_SLIDE))
        return true;
    if (!(Sol_GetActions(world, id) & ACTION_CROUCH))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (world->movements[id].wantsJump)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    if (!Sol_Physx_GetGrounded(world, id))
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return true;
    return false;
}

void Crouch_State_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;

    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->moveState];

    float    x    = Sol_Controller_GetWishdir(world, id).x;
    float    z    = Sol_Controller_GetWishdir(world, id).z;
    vec3s    rot  = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc = {.layerId = ANIM_LAYER_BASE};
    switch (Sol_GetStrafedir(x, z, rot.x, rot.z))
    {
    case STRAFE_FWD:
        desc.anim = ANIM_CROUCHWALK_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_BWD:
    case STRAFE_BWD_RIGHT:
    case STRAFE_BWD_LEFT:
        desc.anim = ANIM_CROUCHWALK_BWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_LEFT:
    case STRAFE_FWD_LEFT:
        desc.anim = ANIM_CROUCHWALK_LEFT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_RIGHT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_CROUCHWALK_RIGHT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    default:
        desc.anim = ANIM_CROUCHWALK_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    }

    const MoveStateForce *forces   = &MOVE_STATE_FORCES[move->configId][move->moveState];
    float                 speedDif = Sol_Physx_GetSpeed(world, id) / forces->speed;
    Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_BASE, speedDif);
}

void Crouch_State_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;

    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->moveState];
    move->targetHeight  = move->baseHeight * 0.65f;
}

void Crouch_State_Exit(World *world, int id)
{

    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->moveState];

    move->targetHeight = move->baseHeight;
}

bool Crouch_State_CanExit(World *world, int id, u32 nextState)
{
    CompMovement *move   = &world->movements[id];
    SolRayResult  result = Sol_Raycast(
        world, (SolRay){.pos = Sol_Xform_GetPos(world, id), .dir = WORLD_UP, .dist = move->baseHeight * 0.6f});

    return !result.hit;
}

bool Crouch_State_CanEnter(World *world, int id, u32 lastState, u32 nextState)
{
    return true;
}
