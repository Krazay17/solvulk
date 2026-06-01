#pragma once
#include "sol/types.h"

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

void Sol_Cam_Update(double dt);

SolCamera *Sol_GetCamera();
vec3s      Sol_Cam_GetRight();