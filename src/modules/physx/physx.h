#pragma once
#include "solmath.h"

typedef struct
{
    vec3 pos;
    vec3 vel;
    float height, radius, mass;
} SolBody;

float Speed(float p, float t)
{
    return p / t;
}

// speed func(p, t) = p / t;

// Speed(5, 5) = 