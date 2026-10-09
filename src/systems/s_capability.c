#include "world.h"

bool Sol_CanAbility(World *world, int id)
{
    if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
        return false;

    return true;
}

bool Sol_CanMove(World *world, int id)
{
    if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
        return false;

    return true;
}