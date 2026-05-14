#pragma once
#include "sol/types.h"

typedef struct CompParent
{
    u32     parentId;
    vec3s   localOffset;
    versors localQuat;
} CompParent;

void Sol_Parent_Init(World *world);
void Sol_Parent_Add(World *world, int id, CompParent desc);
void Sol_Parent_Step(World *world, double dt, double time);
