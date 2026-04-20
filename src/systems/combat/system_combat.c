#include "sol_core.h"
#include <cglm/struct.h>

CompCombat *Sol_Add_Combat(World *world, int id)
{
    world->masks[id] |= HAS_COMBAT;
    world->combats[id] = (CompCombat){0};
    return &world->combats[id];
}

void System_Combat_Tick(World *world, double dt, double time)
{
    int required = HAS_COMBAT;
    int count = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompController *controller = &world->controllers[id];
        CompXform *xform = &world->xforms[id];
        if (controller->actionState & ACTION_ATTACK)
        {
            vec3s endPos = glms_vec3_add(xform->pos, glms_vec3_scale(controller->lookdir, 50.0f));
            Sol_World_Line_Add(world, xform->pos, endPos, (vec3s){1, 1, 0}, (vec3s){0, 1, 0}, 5.0f);
        }
    }
}