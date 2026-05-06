#include "sol_core.h"

void Sol_Movement_Walk_Update(World *world, int id, float dt)
{
    CompMovement   *movement   = &world->movements[id];
    CompController *controller = &world->controllers[id];
    CompBody       *body       = &world->bodies[id];

    if (controller->actionState & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return;
    if (!body->grounded)
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return;
    if (glms_vec3_norm(controller->wishdir) == 0)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;

    AnimDesc desc = {.layerId = ANIM_LAYER_BASE};
    switch (Get_StrafeDir(controller->wishdir.x, controller->wishdir.z, controller->lookdir.x, controller->lookdir.z))
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
}

void Sol_Movement_Walk_Enter(World *world, int id)
{
}

void Sol_Movement_Walk_Exit(World *world, int id)
{
}
bool Sol_Movement_Walk_CanEnter(World *world, int id)
{
    // todo
    return true;
}

bool Sol_Movement_Walk_CanExit(World *world, int id)
{
    // todo
    return true;
}
