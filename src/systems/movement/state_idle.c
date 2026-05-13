#include "sol_core.h"

#include "movement_i.h"

void Sol_Movement_Idle_Update(World *world, int id, float dt)
{
    if (Sol_GetActions(world, id) & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return;
    if (!Sol_Physx_GetGrounded(world, id))
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return;
    if (glms_vec3_norm(Sol_GetWishdir(world, id)) > 0)
        if (Sol_Movement_SetState(world, id, MOVE_WALK))
            return;
}

void Sol_Movement_Idle_Enter(World *world, int id)
{
    if (Sol_GetActions(world, id) & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return;
    if (!Sol_Physx_GetGrounded(world, id))
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return;
    if (glms_vec3_norm(Sol_GetWishdir(world, id)) > 0)
        if (Sol_Movement_SetState(world, id, MOVE_WALK))
            return;

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
