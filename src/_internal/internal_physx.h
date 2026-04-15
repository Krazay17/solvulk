#pragma once

#include <cglm/types-struct.h>

#include "sol/types.h"

typedef struct CompBody CompBody;
typedef struct CompXform CompXform;

#define SOL_PHYS_COLLISION_SKIN 0.01f

#define SPATIAL_NULL 0xFFFFFFFF
#define SPATIAL_SIZE 131072
#define SPATIAL_ENTRIES 65536
#define SPATIAL_STATIC_ENTRIES 1262144
#define SPATIAL_CELL_SIZE 4.0f

typedef struct
{
    vec3s pos, normal, vel;
    int id;
    bool didCollide;
} SolCollision;

typedef struct {
    vec3s a, b, c;
    vec3s normal;
} CollisionTri;

typedef struct
{
    u32 *head;
    u32 *value;
    u32 *next;
    u32 capacity;
    u32 count;
} SpatialTable;

typedef struct WorldSpatial
{
    SpatialTable staticWorld;
    SpatialTable dynamicUnits;
    
    CollisionTri *tris;
    u32 triCount;
} WorldSpatial;


u32 HashCoords(int x, int y, int z);

void SpatialTable_Init(SpatialTable *table, u32 capacity);
void SpatialTable_Clear(SpatialTable *table);
void SpatialTable_Free(SpatialTable *table);
void SpatialTable_Insert(SpatialTable *table, u32 hash, u32 value);
SolCollision ResolveSphereTriangle(CompBody *sphereBody, vec3s *localPos, CollisionTri *tri);

void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform);
void SimpleFloor(CompXform *xform, CompBody *body, float dt);