#pragma once
#include "base.h"

typedef struct World World;

typedef struct CompParent
{
    u32     parentId, active;
    vec3s   localOffset;
    versors localQuat;
} CompParent;
void        Sol_Parent_Init(World *world);
CompParent *Sol_Parent_Add(World *world, int id, int parentId);
void        Sol_Parent_Set(World *world, int id, CompParent desc);
void        Sol_Parent_Step(World *world, double dt, double time);
u32         Sol_Parent_GetParent(World *world, int id);
void        Sol_Parent_SetActive(World *world, int id, bool active);
void        Sol_Parent_SetOffset(World *world, int id);
void        Sol_Parent_SetWithOffset(World *world, int id, int parent);
bool        Sol_Parent_IsActive(World *world, int id);
