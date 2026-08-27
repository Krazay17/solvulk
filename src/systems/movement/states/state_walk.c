#include "movement/si_movement.h"

#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

static bool LeaveState(World *world, int id)
{
    CompMovement   *move       = &world->movements[id];
    CompController *controller = Sol_Controller_Get(world, id);
    if (move->wantsJump)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    if (!Sol_Movement_GetGroundtime(world, id))
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return true;
    if (Sol_Controller_Get(world, id)->actionState & BITC(ACTION_CROUCH))
        if (Sol_Movement_SetState(world, id, MOVE_CROUCH))
            return true;
    if (glms_vec3_norm(controller->wishdir) == 0)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    return false;
}

void Sol_Movement_Walk_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;
    CompMovement *movement = &world->movements[id];

    float    x    = Sol_Controller_Get(world, id)->wishdir.x;
    float    z    = Sol_Controller_Get(world, id)->wishdir.z;
    vec3s    rot  = Sol_RotFromQuat(world->xforms[id].quat);
    movement->stateData[MOVE_WALK].as.walk.strafe = Sol_GetStrafedir(x, z, rot.x, rot.z);
}

void Sol_Movement_Walk_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;
}

void Sol_Movement_Walk_Exit(World *world, int id)
{
}

bool Sol_Movement_Walk_CanExit(World *world, int id, u32 next)
{
    // todo
    return true;
}
bool Sol_Movement_Walk_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    // todo
    return true;
}
