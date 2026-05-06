#pragma once
#include "sol/sol.h"

typedef struct CompController
{
    vec3s              lookdir, wishdir, aimdir, aimpos, aimHitPos;
    vec2s              wishdir2;
    PlayerActionStates actionState;
    float              yaw, pitch;
    float              zoom;
    u32                aimHitEnt;
    ControllerKind     kind;
} CompController;
