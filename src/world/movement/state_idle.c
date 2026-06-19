#include "movement_i.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

static bool LeaveState(World *world, int id)
{
    CompMovement *move = &world->movements[id];
    if (move->wantsJump)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    if (!Sol_Physx_GetGroundtime(world, id))
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

    AnimDesc desc = {.anim = ANIM_IDLE, .layerId = ANIM_LAYER_BASE};
    Sol_Model_PlayAnim(world, id, desc);
}

void Sol_Movement_Idle_Exit(World *world, int id)
{
    // todo
}

bool Sol_Movement_Idle_CanExit(World *world, int id, u32 next)
{
    // todo
    return true;
}

bool Sol_Movement_Idle_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    // todo
    return true;
}
