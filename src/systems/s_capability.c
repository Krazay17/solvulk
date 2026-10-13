#include "world.h"

bool Sol_CanAbility(World *world, int id)
{
    forc(world, id, ScCombat) if (c->is_dead) return false;
    if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
        return false;

    return true;
}

bool Sol_CanMove(World *world, int id)
{
    forc(world, id, ScCombat) if (c->is_dead) return false;
    if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
        return false;

    return true;
}