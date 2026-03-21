#pragma once
#include "solmath.h"

typedef struct
{
    vec3 pos;
    vec3 vel;
    float height, radius, mass;
} SolBody;