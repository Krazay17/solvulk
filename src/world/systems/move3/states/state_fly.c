#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

void Sol_Movement_Fly_Update(World *world, int id, float dt)
{
    // if (!(Sol_Controller_Get(world, id)->actionState(world, id) & ACTION_JUMP))
    //     if (Sol_Move3_SetState(world, id, MOVE_IDLE))
    //         return;
}

void Sol_Movement_Fly_Enter(World *world, int id)
{
}

void Sol_Movement_Fly_Exit(World *world, int id)
{
}


bool Sol_Movement_Fly_CanExit(World *world, int id, u32 next)
{
    return true;
}

bool Sol_Movement_Fly_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    return true;
}
