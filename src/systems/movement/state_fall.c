#include "sol_core.h"

#include "movement_i.h"

static bool LeaveState(World *world, int id)
{
    if (Sol_Controller_IsActionState(world, id, ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_WALLRUN))
            return true;
    if (Sol_Physx_GetGroundtime(world, id))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (Sol_GetActions(world, id) & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    return false;
}

void Sol_Movement_Fall_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;
}

void Sol_Movement_Fall_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;

    AnimDesc desc = {.anim = ANIM_FALL, .layerId = ANIM_LAYER_BASE};
    Sol_Model_PlayAnim(world, id, desc);
}

void Sol_Movement_Fall_Exit(World *world, int id)
{
}

bool Sol_Movement_Fall_CanExit(World *world, int id, u32 next)
{
    return true;
}
bool Sol_Movement_Fall_CanEnter(World *world, int id, u32 last, u32 next)
{
    return true;
}

