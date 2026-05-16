#include "sol_core.h"

#include "movement_i.h"

void Sol_Movement_Walk_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    if (Sol_GetActions(world, id) & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return;
    if (!Sol_Physx_GetGrounded(world, id))
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return;
    if (glms_vec3_norm(Sol_GetWishdir(world, id)) == 0)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;

    AnimDesc desc = {.layerId = ANIM_LAYER_BASE};
    float    x    = Sol_Controller_GetWishdir(world, id).x;
    float    z    = Sol_Controller_GetWishdir(world, id).z;
    vec3s    rot  = Sol_RotFromQuat(world->xforms[id].quat);

    switch (Sol_GetStrafedir(x, z, rot.x, rot.z))
    {
    case STRAFE_FWD:
        desc.anim = ANIM_WALK_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_BWD:
        desc.anim = ANIM_WALK_BWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_LEFT:
        desc.anim = ANIM_WALK_LEFT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_RIGHT:
        desc.anim = ANIM_WALK_RIGHT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    }
    const MoveStateForce *forces   = &MOVE_STATE_FORCES[movement->configId][movement->moveState];
    float                 speedDif = Sol_Physx_GetSpeed(world, id) / forces->speed;
    Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_BASE, speedDif);
}

void Sol_Movement_Walk_Enter(World *world, int id)
{
}

void Sol_Movement_Walk_Exit(World *world, int id)
{
}
bool Sol_Movement_Walk_CanEnter(World *world, int id, int next)
{
    // todo
    return true;
}

bool Sol_Movement_Walk_CanExit(World *world, int id, int next)
{
    // todo
    return true;
}
