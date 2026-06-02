#include "sol_core.h"
#include "xform.h"

void Sol_Xform_Init(World *world)
{
    world->xforms = calloc(MAX_ENTS, sizeof(CompXform));
}

void Xform_Snapshot(World *world)
{
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id                      = world->activeEntities[i];
        world->xforms[id].lastPos   = world->xforms[id].pos;
        world->xforms[id].lastQuat  = world->xforms[id].quat;
        world->xforms[id].lastScale = world->xforms[id].scale;
    }
    if (world->playerID < 0)
        return;
    CompXform *playerXform = &world->xforms[world->playerID];
    Sol_Debug_Add("X", playerXform->pos.x);
    Sol_Debug_Add("Y", playerXform->pos.y);
    Sol_Debug_Add("Z", playerXform->pos.z);
}

void Xform_Interpolate(World *world, float alpha)
{
    int i;
    int count = world->activeCount;
    for (i = 0; i < count; ++i)
    {
        int        id = world->activeEntities[i];
        CompXform *xf = &world->xforms[id];

        xf->drawPos   = glms_vec3_lerp(xf->lastPos, xf->pos, alpha);
        xf->drawQuat  = glms_quat_nlerp(xf->lastQuat, xf->quat, alpha);
        xf->drawScale = glms_vec3_lerp(xf->lastScale, xf->scale, alpha);
    }
}
SolXform Sol_Xform_GetXform(World *world, int id)
{
    CompXform *xform = &world->xforms[id];
    return (SolXform){
        .pos   = xform->pos,
        .scale = xform->scale,
        .quat  = xform->quat,
    };
}

SolXform Sol_Xform_GetDrawXform(World *world, int id)
{
    CompXform *xform = &world->xforms[id];
    return (SolXform){
        .pos   = xform->drawPos,
        .scale = xform->drawScale,
        .quat  = xform->drawQuat,
    };
}

void Sol_Xform_Teleport(World *world, int id, vec3s pos)
{
    world->xforms[id].pos = world->xforms[id].lastPos = world->xforms[id].drawPos = pos;
}

vec3s Sol_Xform_GetPos(World *world, int id)
{
    return world->xforms[id].pos;
}

void Sol_Xform_SetPos(World *world, int id, vec3s pos)
{
    world->xforms[id].pos = pos;
}

void Sol_Xform_SetYaw(World *world, int id, float yaw)
{
    world->xforms[id].quat = Sol_Quat_FromYawPitch(yaw, 0);
}
void Sol_Xform_SetScale(World *world, int id, vec3s scale)
{
    world->xforms[id].scale = scale;
}