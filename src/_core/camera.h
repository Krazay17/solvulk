#pragma once
#include "sol/types.h"

#define MAX_PITCH (GLM_PI_2 - 0.01f)

typedef struct
{
    vec3s anchor, arm;
    float lerpspeed;
    float distance, currentDistance;
    float offset, currentOffset;
    float wallbuffer;
    bool  active;
} SolCameraArm;

void          Sol_Cam_Update(double dt);
SolCameraArm *Sol_Cam_GetArm();
void Sol_Cam_Arm_Update(World *world, vec3s head, double dt);