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
void Sol_Line_Tick(World *world, double dt, double time);
void Sol_Line_Draw(World *world, double dt, double time);
