#pragma once

#include "internal_types.h"
#include "sol/types.h"

#define SOL_COLOR(hex) (SolColor){ \
    .r = ((hex) >> 16) & 0xFF,     \
    .g = ((hex) >> 8) & 0xFF,      \
    .b = ((hex)) & 0xFF,           \
    .a = 255}

#define SOL_COLORA(hex, alpha)     \
    (SolColor)                     \
    {                              \
        .r = ((hex) >> 16) & 0xFF, \
        .g = ((hex) >> 8) & 0xFF,  \
        .b = ((hex)) & 0xFF,       \
        .a = (alpha)               \
    }

SolVec3 Sol_Vec3_Add(SolVec3 a, SolVec3 b);
SolColor Sol_Color_Lerp(SolColor base, SolColor target, float alpha);

bool Sol_Check_2d_Collision(vec2s a, vec4s b);
vec3s ClosestPointOnTriangle(vec3s p, vec3s a, vec3s b, vec3s c);

vec3s Sol_Vec3_FromYawPitch(float yaw, float pitch);
versors Sol_Quat_FromYawPitch(float yaw, float pitch);
versors Sol_Quat_FromLookDir(vec3s lookDir);
versors Sol_Quat_FromLookDira(vec3s lookDir);

float FlashAnim(float dt, float value, float speed);
float PulseAnim(float dt, float value, float speed);