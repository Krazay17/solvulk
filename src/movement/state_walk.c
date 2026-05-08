#include "sol_core.h"

void Sol_Movement_Walk_Update(World *world, int id, float dt)
{
    CompMovement   *movement   = &world->movements[id];
    CompBody       *body       = &world->bodies[id];

    if (Sol_GetActions(world, id) & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return;
    if (!body->grounded)
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return;
    if (glms_vec3_norm(Sol_GetWishdir(world, id)) == 0)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;

    AnimDesc desc = {.layerId = ANIM_LAYER_BASE};
    switch (Sol_GetStrafedir(world, id, 0, 0))
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
