#include "s_ai.h"
#include "controller/s_controller.h"
#include "sol_math.h"
#include "world.h"

void Patrol_State_Update(World *world, int id, float dt)
{
    
}

void Patrol_State_Enter(World *world, int id)
{

}

void Patrol_State_Exit(World *world, int id)
{

}

bool Patrol_State_CanExit(World *world, int id, u32 next)
{
    return true;
}

bool Patrol_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    return true;
}
