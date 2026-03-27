#include "sol_core.h"

static float throttle = 1.0f;
static float accum = 0.0f;

void Sol_System_Info_Tick(World *world, double dt, double time)
{
    if (accum < throttle)
    {
        accum += dt;
        return;
    }
    accum = 0;

    int required = HAS_INFO;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            printf("Info tick %s\n", world->infos[id].name);
            printf("Y = %f\n", world->xforms[id].pos.y);
        }
    }
}