#include "movement_i.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

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


bool Sol_Movement_Fly_CanExit(World *world, int id, u32 next)
{
    return true;
}

bool Sol_Movement_Fly_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    return true;
}
