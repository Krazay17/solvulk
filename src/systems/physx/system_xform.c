#include "sol_core.h"
#include <cglm/struct.h>

void Sol_System_Xform_Snapshot(World *world)
{
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if (world->masks[id] & HAS_XFORM)
        {
            world->xforms[id].lastPos = world->xforms[id].pos;
            world->xforms[id].lastRot = world->xforms[id].rot;
        }
    }
}

// system_xform.c
void Sol_System_Xform_Interpolate(World *world, float alpha)
{
    int i;
    // #pragma omp parallel for
    for (i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if (world->masks[id] & HAS_XFORM)
        {
            CompXform *xf = &world->xforms[id];

            // Branchless lerp
            xf->drawPos.x = xf->lastPos.x + (xf->pos.x - xf->lastPos.x) * alpha;
            xf->drawPos.y = xf->lastPos.y + (xf->pos.y - xf->lastPos.y) * alpha;
            xf->drawPos.z = xf->lastPos.z + (xf->pos.z - xf->lastPos.z) * alpha;

            xf->drawRot = glms_vec4_lerp(xf->lastRot, xf->rot, alpha);
        }
    }
}