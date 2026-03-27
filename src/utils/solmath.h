#pragma once
#include <stdint.h>
#include <cglm/struct.h>
#include <cglm/cglm.h>

typedef struct
{
    float x, y, z;
} SolVec3;

typedef struct
{
    float x, y;
} SolVec2;

typedef struct
{
    float x, y, w, h;
} SolRect;

typedef struct
{
    uint8_t r, g, b, a;
} SolColor;

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

bool Sol_Check_2d_Collision(vec2s a, vec4s b);

SolVec3 Sol_Vec3_Add(SolVec3 a, SolVec3 b);

vec3s Sol_Vec3_FromYawPitch(float yaw, float pitch);

SolColor Sol_Color_Lerp(SolColor base, SolColor target, float alpha);