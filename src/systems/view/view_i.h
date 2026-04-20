#pragma once

#include "sol/types.h"

#define MAX_WORLD_LINES 0xffff

typedef struct SolLine
{
    vec3s a, b;
    vec3s aColor, bColor;
    float ttl;
} SolLine;

typedef struct WorldLines
{
    SolLine lines[MAX_WORLD_LINES];
    int count;
} WorldLines;

void Lines_Init(World *world);