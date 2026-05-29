#pragma once
#include "sol/base.h"

typedef struct CompXform
{
    vec3s   pos, lastPos, drawPos;
    versors quat, lastQuat, drawQuat;
    vec3s   scale, lastScale, drawScale;
} CompXform;

void     Sol_Xform_Init(World *world);
void     Sol_Xform_Add(World *world, int id, vec3s pos);
void     Xform_Snapshot(World *world);
void     Xform_Interpolate(World *world, float alpha);
void     Sol_Xform_Teleport(World *world, int id, vec3s pos);
vec3s    Sol_Xform_GetPos(World *world, int id);
void     Sol_Xform_SetYaw(World *world, int id, float yaw);
void     Sol_Xform_SetPos(World *world, int id, vec3s pos);
void     Sol_Xform_SetScale(World *world, int id, vec3s scale);
SolXform Sol_Xform_GetXform(World *world, int id);
SolXform Sol_Xform_GetDrawXform(World *world, int id);
