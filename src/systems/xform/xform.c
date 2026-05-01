#include "sol_core.h"

CompXform *Sol_Xform_Add(World *world, int id, vec3s pos)
{
    world->xforms[id] = (CompXform){
        .pos     = pos,
        .lastPos = pos,
        .drawPos = pos,
        .scale   = (vec3s){1.0f, 1.0f, 1.0f},
    };
    world->masks[id] |= HAS_XFORM;
    return &world->xforms[id];
}

void Xform_Snapshot(World *world)
{
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if (world->masks[id] & HAS_XFORM)
        {
            vec3s pos = world->xforms[id].pos;
            if (isnan(pos.x) || isnan(pos.y) || isnan(pos.z))
            {
                pos.x = 0;
                pos.y = 5;
                pos.z = 0;
            }
            world->xforms[id].lastPos   = world->xforms[id].pos;
            world->xforms[id].lastQuat  = world->xforms[id].quat;
            world->xforms[id].lastScale = world->xforms[id].scale;
        }
    }
    if (world->playerID < 0)
        return;
    CompXform *playerXform = &world->xforms[world->playerID];
    Sol_Debug_Add("X", playerXform->pos.x);
    Sol_Debug_Add("Y", playerXform->pos.y);
    Sol_Debug_Add("Z", playerXform->pos.z);
}

// system_xform.c
void Xform_Interpolate(World *world, float alpha)
{
    int i;
    int count = world->activeCount;
    for (i = 0; i < count; ++i)
    {
        int id = world->activeEntities[i];
        if (world->masks[id] & HAS_XFORM)
        {
            CompXform *xf = &world->xforms[id];

            xf->drawPos   = glms_vec3_lerp(xf->lastPos, xf->pos, alpha);
            xf->drawQuat  = glms_quat_nlerp(xf->lastQuat, xf->quat, alpha);
            xf->drawScale = glms_vec3_lerp(xf->lastScale, xf->scale, alpha);
        }
    }
}

void Xform_Teleport(CompXform *xform, vec3s pos)
{
    xform->pos = xform->lastPos = xform->drawPos = pos;
}