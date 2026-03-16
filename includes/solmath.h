#pragma once
#include <stdbool.h>
#include <math.h>
#include <cglm/cglm.h>

typedef struct
{
    float x, y, w, h;
} SolRect;

typedef struct
{
    float r, g, b, a;
} SolColor;

bool Sol_Check_2d_Collision(vec2 a, SolRect b);