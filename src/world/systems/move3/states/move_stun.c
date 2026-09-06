#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

void Move_Stun_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
}

void Move_Stun_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

void Move_Stun_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

bool Move_Stun_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    if (next == MOVE_DEAD) 
        return true;
        
    if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
        return false;

    return true;
}

bool Move_Stun_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if(Sol_Comp_Has(world, id, ScBuff))
    {
        ScBuff *buff = Sol_Comp_Get(world, id, ScBuff);
        return buff->activeKindsMask & BITC(BUFFKIND_STUN);
    }
    return false;
}
