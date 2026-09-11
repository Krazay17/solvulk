#include "ability/si_ability.h"
#include "world.h"

void Ability_Idle_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
}
void Ability_Idle_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    ability->activeSlot = -1;
}
void Ability_Idle_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
}
bool Ability_Idle_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    return true;
}
bool Ability_Idle_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    return true;
}
