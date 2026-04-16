// internal_physx.h
#pragma once

#include "internal_types.h"
#include "sol/types.h"

typedef struct CompBody CompBody;
typedef struct CompXform CompXform;

#define SOL_TIMESTEP 1.0 / 60.0


#define SOL_PHYS_COLLISION_SKIN 0.01f

#define SPATIAL_NULL 0xFFFFFFFF
#define SPATIAL_SIZE 131072
#define SPATIAL_ENTRIES 65536
#define SPATIAL_STATIC_ENTRIES 0x00FFFFFF
#define SPATIAL_CELL_SIZE 2.0f

u32 HashCoords(int x, int y, int z);
vec3s ClosestPointOnTriangle(vec3s p, vec3s a, vec3s b, vec3s c);

void SpatialTable_Init(SpatialTable *table, u32 capacity);
void SpatialTable_Clear(SpatialTable *table);
void SpatialTable_Free(SpatialTable *table);
void SpatialTable_Insert(SpatialTable *table, u32 hash, u32 value);
SolCollision ResolveSphereTriangle(CompBody *sphereBody, vec3s *localPos, CollisionTri *tri);

void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform);
void SimpleFloor(CompXform *xform, CompBody *body, float dt);

SpatialCell GetSpatialCell(vec3s pos);
void Sol_Spatial_Build_Dynamic(World *world);
void ResolvePositionOnly(CompBody *aBody, CompXform *aXform,
                         CompBody *bBody, CompXform *bXform);
void ResolveVelocityOnly(CompBody *aBody, CompXform *aXform,
                         CompBody *bBody, CompXform *bXform);