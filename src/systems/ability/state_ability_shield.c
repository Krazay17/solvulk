#include "sol_core.h"

#include "ability_i.h"

void Shield_State_Update(World *world, int id, float dt)
{
    Sol_Movement_SetSpeedMod(world, id, 0.5f);
}

void Shield_State_Enter(World *world, int id)
{
}

void Shield_State_Exit(World *world, int id)
{
}

bool Shield_State_CanEnter(World *world, int id, int last)
{
    return true;
}

bool Shield_State_CanExit(World *world, int id, int next)
{
    return true;
}
