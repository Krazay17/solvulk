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
    if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
        return false;

    return true;
}

bool Move_Stun_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    forc(world, id, ScBuff) return c->activeKindsMask & (1ULL << BUFFKIND_STUN);
    return false;
}

const MoveStateFuncs move_stun_funcs = {
    .update   = Move_Stun_Update,
    .enter    = Move_Stun_Enter,
    .exit     = Move_Stun_Exit,
    .canExit  = Move_Stun_CanExit,
    .canEnter = Move_Stun_CanEnter,
};
