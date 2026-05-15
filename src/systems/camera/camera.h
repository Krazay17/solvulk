#pragma once
#include "sol/types.h"

typedef struct
{
    vec3s anchor, arm;
    float lerpspeed;
    float distance, currentDistance;
    float offset, currentOffset;
    float wallbuffer;
    bool  active;
} SolCameraArm;

typedef struct SolCamera
{
    mat4  proj;
    mat4  view;
    mat4  viewProj;
    vec3  position;
    vec3  target;
    float fov;
    float nearClip;
    float farClip;
} SolCamera;

void Sol_Cam_Init(World *world);
void Sol_Cam_Update(double dt);

void Sol_Cam_Arm_Update(World *world, vec3s head, double dt);

SolCamera    *Sol_GetCamera();
SolCameraArm *Sol_Cam_GetArm();
