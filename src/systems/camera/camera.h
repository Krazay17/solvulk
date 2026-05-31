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
    mat4s proj;
    mat4s view;
    mat4s viewProj;
    vec3s position, anchor;
    vec3s target, dir;
    vec3s up;
    float fov;
    float nearClip;
    float farClip;
} SolCamera;

void Sol_Cam_Init(World *world);
void Sol_Cam_Update(World *world, double dt);

void Sol_Cam_Arm_Update(World *world, vec3s head, double dt);

SolCamera    *Sol_GetCamera();
SolCameraArm *Sol_Cam_GetArm();
void          Sol_Cam_SetActivecam(World *world);
SolCamera    *Sol_Cam_GetCam(World *world);
void          Sol_Cam_GetRight(vec3s *right);