#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

void Stun_State_Update(World *world, int id, float dt)
{
}
void Stun_State_Enter(World *world, int id)
{
}
void Stun_State_Exit(World *world, int id)
{
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
