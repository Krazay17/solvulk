#include "sol_core.h"

#include "movement_i.h"

#define REMOVE_PHYSX_TIMER 10.0f
#define DESTROY_TIMER 25.0f

void Dead_State_Update(World *world, int id, float dt)
{
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[movement->state];
    data->elapsed += dt;
    if (data->elapsed > REMOVE_PHYSX_TIMER)
    {
        world->masks[id] &= ~HAS_BODY3;
        world->masks[id] &= ~HAS_BODY2;
    }
    if (data->elapsed > DESTROY_TIMER)
        if (!world->vitals[id].doesRespawn)
            Sol_Destroy_Ent(world, id);
}
void Dead_State_Enter(World *world, int id)
{
    Sol_Model_PlayAnim(
        world, id,
        (AnimDesc){.anim = ANIM_DEATH, .layerId = ANIM_LAYER_OVERRIDE, .speed = 0.6f, .oneShot = true, .noFade = true});
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
