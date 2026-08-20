#pragma once
#include "sol/types.h"

#define MAX_LOCAL_PLAYERS 2

typedef struct
{
    int localIdx;
} CompPlayer;

void Sol_Player_Init(World *world);

CompPlayer *Sol_Player_Add(World *world, int id, int idx);
CompPlayer *Sol_Player_Get(World *world, int id);
void        Sol_Player_Remove(World *world, int id);

int Sol_Player_GetEnt(World *world, int localIdx);
