#include "sol_core.h"

#include "ability_i.h"

void IdleAbility_State_Update(World *world, int id, float dt)
{
}
void IdleAbility_State_Enter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    ability->activeSlot = -1;
}
void IdleAbility_State_Exit(World *world, int id)
{
}
bool IdleAbility_State_CanExit(World *world, int id, u32 next)
{
    return true;
}
bool IdleAbility_State_CanEnter(World *world, int id, u32 last, u32 next)
{
    return true;
}
