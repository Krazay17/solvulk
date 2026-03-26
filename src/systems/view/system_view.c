#include "sol_core.h"

void Sol_System_Update_View(World *world, double dt, double time)
{
    int required = HAS_XFORM | HAS_SHAPE;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            float width = world->shapes[id].width;
            float height = world->shapes[id].height;
            Sol_Draw_Rectangle((SolRect){.x = world->xforms[id].pos[0], .y = world->xforms[id].pos[1], .w = width, .h = height},
                               (SolColor){0, 255, 0, 255},
                               0);
        }
    }
}