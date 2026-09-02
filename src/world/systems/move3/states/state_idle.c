#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

static bool LeaveState(World *world, int id, ScMove3 *move, ScController *cont)
{
    if (move->wantsJump)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    if (move->airtime > 0)
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return true;
    if (cont->actionState & BITC(ACTION_CROUCH))
        if (Sol_Movement_SetState(world, id, MOVE_CROUCH))
            return true;
    if (glms_vec3_norm(cont->wishdir) > 0)
        if (Sol_Movement_SetState(world, id, MOVE_WALK))
            return true;
    return false;
}

void Sol_Movement_Idle_Update(World *world, int id, float dt)
{
    ScMove3   *move = Sol_Comp_Get(world, id, ScMove3);
    ScController *cont = Sol_Comp_Get(world, id, ScController);
    if (LeaveState(world, id, move, cont))
        return;

    move->gravityMod = 0.0f;
    // if (Sol_Physx_GetSpeed(world, id) < 0.5f)
    // {
    //     Sol_Physx_AddVel(
    //         world, id, vecSca(Sol_Physx_GetGround(world, id), MOVE_STATE_FORCES[move->kind][move->state].gravity *
    //         dt));
    // }
}

void Sol_Movement_Idle_Enter(World *world, int id)
{
    ScMove3   *move = Sol_Comp_Get(world, id, ScMove3);
    ScController *cont = Sol_Comp_Get(world, id, ScController);
    if (LeaveState(world, id, move, cont))
        return;

    move->targetHeight = move->baseHeight;
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
