#include "world.h"

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind)
{
    ScBuff *buff = Sol_Comp_Get(world, id, ScBuff);
    if (!buff)
        return false;
    return buff->activeKindsMask & BITC(kind);
}