#include "movement_i.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

#define REMOVE_PHYSX_TIMER 20.0f
#define DESTROY_TIMER 125.0f

void Dead_State_Update(World *world, int id, float dt)
{
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[movement->state];

    if (data->elapsed > DESTROY_TIMER)
    {
        Sol_Destroy_Ent(world, id);
        return;
    }
    if (data->elapsed > REMOVE_PHYSX_TIMER)
    {
        world->masks[id] &= ~BITC(HAS_BODY3);
        world->masks[id] &= ~BITC(HAS_BODY2);
    }
}

void Dead_State_Enter(World *world, int id)
{
    Sol_Model_PlayAnim(
        world, id,
        (AnimDesc){.anim = ANIM_DEATH, .layerId = ANIM_LAYER_OVERRIDE, .speed = 1.0f, .playKind = ANIMPLAYKIND_NOLOOP});
    world->movements[id].targetHeight = world->movements[id].baseHeight * 0.6f;
}
void Dead_State_Exit(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_OVERRIDE});
    world->movements[id].targetHeight = world->movements[id].baseHeight;
}
bool Dead_State_CanExit(World *world, int id, u32 next)
{
    return next != MOVE_DEAD;
}
bool Dead_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    return true;
}
