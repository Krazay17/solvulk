#pragma once
#include "sol/types.h"

typedef struct
{
    vec3s a, b;
    float ttl;
    vec4s colorA, colorB;
} LineDesc;

void Sol_Line_Init(World *world);

void Sol_Line_Add(World *world, LineDesc desc);
