#pragma once
#include "sol/types.h"

typedef struct
{
    int remoteId;
} CompRemote;

void        Sol_Remote_Init(World *world);
CompRemote *Sol_Remote_Add(World *world, int id);
CompRemote *Sol_Remote_Get(World *world, int id);
void        Sol_Remote_Remove(World *world, int id);