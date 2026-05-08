#include "sol_core.h"

void Sol_Movement_Fly_Update(World *world, int id, float dt)
{
    if (!(Sol_GetActions(world, id) & ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
}

void Sol_Movement_Fly_Enter(World *world, int id)
{
    AnimDesc desc = {.anim = ANIM_FALL};
    Sol_Model_PlayAnim(world, id, desc);
}

void Sol_Movement_Fly_Exit(World *world, int id)
{
}

bool Sol_Movement_Fly_CanEnter(World *world, int id, int last)
{
    return true;
}

bool Sol_Movement_Fly_CanExit(World *world, int id, int next)
{
    return true;
}
