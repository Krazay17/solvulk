#pragma once

#include "sol/types.h"

#define MAX_WORLD_LINES 0xffff


typedef struct WorldLines
{
    SolLine lines[MAX_WORLD_LINES];
    int count;
} WorldLines;
