#include "ai/si_ai.h"

#include "sol_math.h"
#include "world.h"
#include "controller/s_controller.h"

void Idle_State_Update(World *world, int id, float dt)
{
    CompAi *ai = Sol_Ai_Get(world, id);
    ai->target = Sol_Ai_FindTarget(world, id);

    if (ai->target)
    {
        Sol_Ai_SetState(world, id, AISTATE_AGGRO, 1);
        return;
    }
}

void Idle_State_Enter(World *world, int id)
{
    CompController *controller = Sol_Controller_Get(world, id);
    controller->wishdir        = (vec3s){0};
    controller->actionState    = 0;
}

void Idle_State_Exit(World *world, int id)
{
}

bool Idle_State_CanExit(World *world, int id, u32 next)
{
    return true;
}

bool Idle_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    return true;
}
