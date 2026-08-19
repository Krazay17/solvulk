#pragma once
#include "base.h"
#include "sol_math.h"

#define MAX_CAMERA_ZOOM 100.0f

typedef struct SolCamera
{
    mat4s proj;
    mat4s view;
    mat4s viewProj;
    vec3s pos, anchor;
    vec3s target, dir;
    vec3s up;
    float fov;
    float nearClip;
    float farClip;
    float lerpspeed;
    float roll;
    float distance, currentDistance;
    float offset, currentOffset;
} SolCamera;

extern SolCamera solCamera;

static inline void Sol_Camera_AdjustDistance(SolCamera *cam, float delta)
{
    cam->distance = glm_clamp(cam->distance + delta, 0.0f, MAX_CAMERA_ZOOM);
}

SolCamera *Sol_Cam_GetActive(World *world);
vec3s      Sol_Cam_GetRight();
vec3s      Sol_Cam_GetFwd();
vec3s      Sol_Cam_GetPos();
mat4s      Sol_Cam_GetViewProj();
