#pragma once
#include "sol/types.h"

typedef struct CompOwner
{
    u32 ownerId;
    u32 team;
} CompOwner;

typedef struct
{
    u32 ownerId, team;
} OwnerDesc;

void Sol_Owner_Init(World *world);

void Sol_Owner_Add(World *world, int id, OwnerDesc desc);
void Sol_Owner_SetOwner(World *world, int id, int ownerId);
u32  Sol_Owner_GetTeam(World *world, int id);
bool Sol_Owner_GetHostile(World *world, int id, int target);