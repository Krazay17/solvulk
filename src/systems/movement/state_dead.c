#include "sol_core.h"

#include "movement_i.h"

void Dead_State_Update(World *world, int id, float dt)
{

}
void Dead_State_Enter(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, (AnimDesc){.anim = ANIM_DEATH, .layerId = ANIM_LAYER_OVERRIDE, .speed = 0.5f});
}
void Dead_State_Exit(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_OVERRIDE});
}
bool Dead_State_CanExit(World *world, int id, u32 next)
{
    return next != MOVE_DEAD;
}
bool Dead_State_CanEnter(World *world, int id, u32 last, u32 next)
{
    return true;
}
