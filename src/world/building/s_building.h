#pragma once
#include "sol/types.h"

// typedef struct CompBuilding
// {
//     BuildingKind   kind;
//     bool           doesSnap;
//     vec3s          placePos, prevPos;
//     versors        placeRot;
//     SolModelHandle model;
// } CompBuilding;

typedef struct CompBuilder
{
    bool    placing, doesSnap;
    float   scale;
    vec3s   placePos;
    versors placeRot;
    u32     model;
} CompBuilder;

void Sol_Builder_Init(World *world);
void Sol_Builder_Add(World *world, int id, u32 model);
void Sol_Builder_PlaceBuilding(World *world, int id);
void Sol_Builder_RotatePlacing(World *world, int id, float yawDelta);
