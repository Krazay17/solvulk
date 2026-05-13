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

void Sol_Camera_Tick(World *world, double dt, double time, float alpha);
void Sol_Cam_Arm_Update(World *world, vec3s head, double dt);
void Sol_Crosshair_Draw(World *world, double dt, double time);

SolCamera    *Sol_GetCamera();
SolCameraArm *Sol_Cam_GetArm();
