#pragma once
#include <stdbool.h>

typedef struct
{
    float x, y;
} Vec2;

typedef struct
{
    float x, y, z;
} Vec3;

typedef struct
{
    float x, y, z, w;
} Quat;

typedef struct
{
    Vec3 pos;
    Quat quat;
} Xform;

typedef struct
{
    float x, y, w, h;
} Rect;

bool Sol_Check_2d_Collision(Vec2 a, Rect b);