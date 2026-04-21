#include "sol_core.h"
#include <cglm/struct.h>

static SolRay rayTest;

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
            SolRayResult result = Sol_RaycastD(world, (SolRay){.pos = xform->pos, .dir = controller->lookdir, .dist = 50.0f});
            Sol_Debug_Add("Ray", result.dist);
        }
    }
}