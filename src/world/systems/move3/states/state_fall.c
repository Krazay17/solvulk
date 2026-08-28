#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

static bool LeaveState(World *world, int id, SolMove3 *move, SolController *controller)
{
    if (move->groundtime > 0)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (controller->actionState & BITC(ACTION_CROUCH))
        if (Sol_Movement_SetState(world, id, MOVE_SLIDE))
            return true;
    if (controller->actionState & BITC(ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_WALLRUN))
            return true;
    if (controller->actionState & BITC(ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    return false;
}

void Sol_Movement_Fall_Update(World *world, int id, float dt)
{
    SolMove3      *move       = Sol_Comp_Get(world, id, SolMove3);
    SolController *controller = Sol_Comp_Get(world, id, SolController);
    if (LeaveState(world, id, move, controller))
        return;
}

void Sol_Movement_Fall_Enter(World *world, int id)
{
    SolMove3      *move       = Sol_Comp_Get(world, id, SolMove3);
    SolController *controller = Sol_Comp_Get(world, id, SolController);
    if (LeaveState(world, id, move, controller))
        return;
}

void Sol_Movement_Fall_Exit(World *world, int id)
{
}

bool Sol_Movement_Fall_CanExit(World *world, int id, u32 next)
{
    return true;
}
bool Sol_Movement_Fall_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    return true;
}
