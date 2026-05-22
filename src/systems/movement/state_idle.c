#include "sol_core.h"

#include "movement_i.h"

static bool LeaveState(World *world, int id)
{
    if (Sol_GetActions(world, id) & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    if (!Sol_Physx_GetGrounded(world, id))
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return true;
    if (Sol_GetActions(world, id) & ACTION_CROUCH)
        if (Sol_Movement_SetState(world, id, MOVE_CROUCH))
            return true;
    if (glms_vec3_norm(Sol_GetWishdir(world, id)) > 0)
        if (Sol_Movement_SetState(world, id, MOVE_WALK))
            return true;
    return false;
}

void Sol_Movement_Idle_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;
}

void Sol_Movement_Idle_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;

    CompMovement *move = &world->movements[id];
    move->targetHeight = move->baseHeight;

    AnimDesc desc = {.anim = ANIM_IDLE, .blendIn = 3.0f, .layerId = ANIM_LAYER_BASE};
    Sol_Model_PlayAnim(world, id, desc);
}

void Sol_Movement_Idle_Exit(World *world, int id)
{
    // todo
}

bool Sol_Movement_Idle_CanEnter(World *world, int id, int last)
{
    // todo
    return true;
}

bool Sol_Movement_Idle_CanExit(World *world, int id, int next)
{
    // todo
    return true;
}
