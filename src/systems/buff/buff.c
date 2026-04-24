#include "sol_core.h"
#include <cglm/struct.h>

CompDebuff *Sol_Debuff_Add(World *world, int id, CompDebuff init)
{
    CompDebuff debuff = init;

    world->debuffs[id] = debuff;
    world->masks[id] |= HAS_DEBUFF;
    return &world->debuffs[id];
}

void Sol_Debuff_Remove(World *world, int id)
{
    memset(&world->debuffs[id], 0, sizeof(CompDebuff));
    world->masks[id] &= ~HAS_DEBUFF;
}

void Debuff_Step(World *world, double dt, double time)
{
    int required = HAS_DEBUFF;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompDebuff *debuff = &world->debuffs[id];
        debuff->duration -= dt;
        if (debuff->duration <= 0)
            Sol_Debuff_Remove(world, id);

        if (debuff->kinds & DEBUFF_KNOCKBACK)
        {
            CompBody *body = &world->bodies[id];
            body->vel      = glms_vec3_scale(debuff->hit.dir, debuff->hit.power);
        }
    }
}