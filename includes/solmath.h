#pragma once
#include <stdbool.h>

typedef struct
{
    float x, y;
} Vec2;

typedef struct
{
    float x, y, w, h;
} Rect;

bool Sol_Check_2d_Collision(Vec2 a, Rect b);