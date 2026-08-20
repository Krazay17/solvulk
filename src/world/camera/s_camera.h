#pragma once
#include "sol/types.h"

#define MAX_CAMERA_ZOOM 100.0f
#define CAMERA_LERP_SPEED 10.0f

typedef enum
{
    CAMKIND_3D,
    CAMKIND_NOARM,
    CAMKIND_COUNT,
} CamKind;

typedef struct CompCam
{
    vec3s pos, anchor;
    vec3s target, dir;
    vec3s up;
    float fov;
    float lerpspeed;
    float roll;
    float distance, currentDistance;
    float offset, currentOffset;
} CompCam;

void Sol_Cam_Init(World *world);

CompCam *Sol_Cam_Add(World *world, int id, CamKind kind, bool active);
CompCam *Sol_Cam_Get(World *world, int id);
void     Sol_Cam_Remove(World *world, int id);

void Sol_Cam_AdjustDistance(World *world, int id, float delta);

vec3s Sol_Cam_GetPos();
mat4s Sol_Cam_GetViewProj();
vec3s Sol_Cam_GetRight();
vec3s Sol_Cam_GetFwd();
