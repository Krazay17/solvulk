#include "world.h"
#include "estate.h"

void Ability_Idle_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
}
void Ability_Idle_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
}
void Ability_Idle_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
}
bool Ability_Idle_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    return true;
}
bool Ability_Idle_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    return true;
}

const AbilityStateFunc ability_idle_state = {
    .update   = Ability_Idle_Update,
    .enter    = Ability_Idle_Enter,
    .exit     = Ability_Idle_Exit,
    .canExit  = Ability_Idle_CanExit,
    .canEnter = Ability_Idle_CanEnter,
};
