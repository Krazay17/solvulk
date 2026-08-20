#pragma once
#include "sol/types.h"

typedef struct
{
    int localIdx;
} CompPlayer;

void Sol_Player_Init(World *world);

CompPlayer *Sol_Player_Add(World *world, int id);
void        Sol_Player_Remove(World *world, int id);