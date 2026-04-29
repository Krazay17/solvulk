#include "sol_core.h"

CompBuff *Sol_Buff_Add(World *world, int id, CompBuff init)
{
    CompBuff debuff = init;

    world->buffs[id] = debuff;
    world->masks[id] |= HAS_BUFF;
    return &world->buffs[id];
}

void Sol_Debuff_Remove(World *world, int id)
{
    memset(&world->buffs[id], 0, sizeof(CompBuff));
    world->masks[id] &= ~HAS_BUFF;
}

void Buff_Step(World *world, double dt, double time)
{
    int required = HAS_BUFF;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompBuff *buff = &world->buffs[id];
        buff->duration -= dt;
        if (buff->duration <= 0)
            Sol_Debuff_Remove(world, id);

        if (buff->kind & BUFF_KNOCKBACK)
        {
            CompBody *body = &world->bodies[id];
            body->vel      = glms_vec3_scale(buff->hit.dir, buff->hit.power);
        }
    }
}