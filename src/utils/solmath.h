#pragma once
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <cglm/types.h>
#include <cglm/struct.h>

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

bool Sol_Check_2d_Collision(vec2 a, SolRect b);

SolVec3 Sol_Vec_Add(vec3 a, vec3 b);