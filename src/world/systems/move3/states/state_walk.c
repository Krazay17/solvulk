#include "move3/s_move3.h"

#include "world.h"
#include "sol_math.h"

static bool LeaveState(World *world, int id, ScMove3 *move, ScController *cont)
{
    if (move->wantsJump)
        if (Sol_Move3_SetState(world, id, MOVE_JUMP))
            return true;
    if (move->airtime > 0)
        if (Sol_Move3_SetState(world, id, MOVE_FALL))
            return true;
    if (cont->actionState & BITC(ACTION_CROUCH))
        if (Sol_Move3_SetState(world, id, MOVE_CROUCH))
            return true;
    if (glms_vec3_norm(cont->wishdir) == 0)
        if (Sol_Move3_SetState(world, id, MOVE_IDLE))
            return true;
    return false;
}

void Sol_Movement_Walk_Update(World *world, int id, float dt)
{
    ScMove3   *move  = Sol_Comp_Get(world, id, ScMove3);
    ScController *cont  = Sol_Comp_Get(world, id, ScController);
    ScXform      *xform = Sol_Comp_Get(world, id, ScXform);
    if (LeaveState(world, id, move, cont))
        return;

    float x                                   = cont->wishdir.x;
    float z                                   = cont->wishdir.z;
    vec3s rot                                 = Sol_RotFromQuat(xform->rot);
    move->stateData[MOVE_WALK].as.walk.strafe = Sol_GetStrafedir(x, z, rot.x, rot.z);
}

void Sol_Movement_Walk_Enter(World *world, int id)
{
    ScMove3   *move  = Sol_Comp_Get(world, id, ScMove3);
    ScController *cont  = Sol_Comp_Get(world, id, ScController);
    if (LeaveState(world, id, move, cont))
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
