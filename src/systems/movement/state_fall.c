#include "sol_core.h"
#include "movement_system.h"

void Sol_Movement_Fall_Update(World *world, int id, float dt)
{
    CompBody *body = &world->bodies[id];
    if (body->grounded)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
    // if (controller->actionState & ACTION_JUMP)
    //     if (Sol_Movement_SetState(world, id, MOVE_FLY))
    //         return;
}

void Sol_Movement_Fall_Enter(World *world, int id)
{
    AnimDesc desc = {.anim = ANIM_FALL, .layerId = ANIM_LAYER_BASE};
    Sol_Model_PlayAnim(world, id, desc);
}

void Sol_Movement_Fall_Exit(World *world, int id)
{
}

bool Sol_Movement_Fall_CanEnter(World *world, int id)
{
    return true;
}

bool Sol_Movement_Fall_CanExit(World *world, int id)
{
    return true;
}
