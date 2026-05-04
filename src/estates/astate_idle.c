#include "ability_states.h"
#include "sol_core.h"

void IdleAbility_State_Update(World *world, int id, float dt)
{
    printf("IdleTick");
}

void IdleAbility_State_Enter(World *world, int id)
{
    
}
void IdleAbility_State_Exit(World *world, int id)
{
}
bool IdleAbility_State_CanEnter(World *world, int id)
{
    return true;
}
bool IdleAbility_State_CanExit(World *world, int id)
{
    return true;
}
