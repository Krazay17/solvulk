#pragma once

#include <cglm/types-struct.h>

#include "sol/types.h"

typedef struct CompBody CompBody;
typedef struct CompXform CompXform;

#define SOL_PHYS_COLLISION_SKIN 0.01f

#define SPATIAL_NULL 0xFFFFFFFF
#define SPATIAL_SIZE 32768
#define SPATIAL_ENTRIES 32768
#define SPATIAL_STATIC_ENTRIES 131072
#define SPATIAL_CELL_SIZE 2.0f

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

//void Spatial_Insert(SpatialTable *table, vec3s pos, CompBody *body, u32 value);
//void ResolveWorldCollisions(World *world);
//void ResolveWorldMeshSimple(CompBody *sphereBody, CompXform *sphereXform, CompXform *meshXform, SolModel *model);

// void BuildSpatialHash(World *world, SpatialHash *grid);
// void ResolveCollisionsSpatial(World *world, SpatialHash *grid);
// TriAABB Sol_GetTriAABB(vec3s a, vec3s b, vec3s c);

// #define CELL_SIZE 2.0f
// #define MAX_ENTITIES_PER_CELL 32
// #define TABLE_SIZE 8192

// #define WORLD_CELL_SIZE 4.0f
// #define WORLD_TABLE_SIZE 16384
// #define MAX_TRIS_PER_CELL 64

// typedef struct
// {
//     float minX, maxX;
//     float minY, maxY;
//     float minZ, maxZ;
// } TriAABB;

// typedef struct
// {
//     vec3s a, b, c;
//     vec3s normal;
// } CollisionTri;

// typedef struct
// {
//     uint32_t triIndices[MAX_TRIS_PER_CELL];
//     uint32_t count;
// } WorldCell;

// typedef struct
// {
//     CollisionTri *tris;
//     uint32_t triCount;
//     WorldCell cells[WORLD_TABLE_SIZE];
// } WorldCollider;

// typedef struct
// {
//     int entityIds[MAX_ENTITIES_PER_CELL];
//     int count;
// } SpatialCell;

// typedef struct
// {
//     SpatialCell cells[TABLE_SIZE];
// } SpatialHash;