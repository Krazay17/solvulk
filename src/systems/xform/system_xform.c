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
            world->xforms[id].lastQuat = world->xforms[id].quat;
            world->xforms[id].lastScale = world->xforms[id].scale;
        }
    }
    CompXform *playerXform = &world->xforms[world->playerID];
    Sol_Debug_Add("X", playerXform->pos.x);
    Sol_Debug_Add("Y", playerXform->pos.y);
    Sol_Debug_Add("Z", playerXform->pos.z);
}

// system_xform.c
void Sol_System_Xform_Interpolate(World *world, float alpha)
{
    int i;
    int count = world->activeCount;
    for (i = 0; i < count; ++i)
    {
        int id = world->activeEntities[i];
        if (world->masks[id] & HAS_XFORM)
        {
            CompXform *xf = &world->xforms[id];

            xf->drawPos = glms_vec3_lerp(xf->lastPos, xf->pos, alpha);

            xf->drawQuat = glms_quat_slerp(xf->lastQuat, xf->quat, alpha);

            xf->drawScale = glms_vec3_lerp(xf->lastScale, xf->scale, alpha);
        }
    }
}