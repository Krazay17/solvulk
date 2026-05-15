#pragma once
#include "sol/types.h"

typedef struct CompOwner
{
    u32 ownerId;
} CompOwner;

void Sol_Owner_Init(World *world);

void Sol_Owner_Add(World *world, int id, int ownerId);
u32 Sol_Owner_GetOwner(World *world, int id);