#include "movement/si_movement.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"
#include "buff/s_buff.h"

void Stun_State_Update(World *world, int id, float dt)
{
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[movement->state];
    
}
void Stun_State_Enter(World *world, int id)
{
}
void Stun_State_Exit(World *world, int id)
{
    Sol_Model_StopAnim(world, id, ANIM_LAYER_OVERRIDE, 0);
}
bool Stun_State_CanExit(World *world, int id, u32 next)
{
    if (next == MOVE_DEAD) 
        return true;
        
    if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
        return false;

    return true;
}
bool Stun_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    return true;
}
