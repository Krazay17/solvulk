#include "world.h"

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind)
{
    SolBuff *buff = Sol_Comp_Get(world, id, SolBuff);
    if (!buff)
        return false;
    return buff->activeKindsMask & BITC(kind);
}