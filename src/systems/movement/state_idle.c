#include "movement_system.h"
#include "sol_core.h"

void Sol_Movement_Idle_Update(World *world, int id, float dt)
{
    CompController       *controller = &world->controllers[id];
    CompBody             *body       = &world->bodies[id];

    if (controller->actionState & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return;
    if (!body->grounded)
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return;
    if (glms_vec3_norm(controller->wishdir) > 0)
        if (Sol_Movement_SetState(world, id, MOVE_WALK))
            return;
}

void Sol_Movement_Idle_Enter(World *world, int id)
{
    CompController *controller = &world->controllers[id];
    CompBody       *body       = &world->bodies[id];

    if (controller->actionState & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return;
    if (!body->grounded)
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return;
    if (glms_vec3_norm(controller->wishdir) > 0)
        if (Sol_Movement_SetState(world, id, MOVE_WALK))
            return;

            AnimDesc desc = {.anim = ANIM_IDLE, .blendIn = 3.0f, .layerId = ANIM_LAYER_BASE};
    Sol_Model_PlayAnim(world, id, desc);
}

void Sol_Movement_Idle_Exit(World *world, int id)
{
    // todo
}

bool Sol_Movement_Idle_CanEnter(World *world, int id)
{
    // todo
    return true;
}

bool Sol_Movement_Idle_CanExit(World *world, int id)
{
    // todo
    return true;
}
