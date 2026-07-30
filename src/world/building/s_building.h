#pragma once
#include "sol/types.h"

typedef struct CompBuilding
{
    BuildingKind kind;
    bool         placing, placed, doesSnap;
    vec3s        placePos, prevPos;
} CompBuilding;

void Sol_Building_Init(World *world);
void Sol_Building_Add(World *world, int id, bool placed);
