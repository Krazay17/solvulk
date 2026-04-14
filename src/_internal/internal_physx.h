#pragma once

#include <cglm/types-struct.h>
#include "sol/types.h"

typedef struct World World;
typedef struct CompBody CompBody;
typedef struct CompXform CompXform;
typedef struct CompModel CompModel;

#define CELL_SIZE 2.0f
#define MAX_ENTITIES_PER_CELL 32
#define TABLE_SIZE 8192

#define WORLD_CELL_SIZE 4.0f
#define WORLD_TABLE_SIZE 16384
#define MAX_TRIS_PER_CELL 64
#define SOL_PHYS_COLLISION_SKIN 0.01f

#define SOL_SPATIAL_NULL 0xFFFFFFFF
#define SPATIAL_SIZE 32768
#define SPATIAL_ENTRIES 32768
#define SPATIAL_CELL_SIZE 2.0f

typedef struct
{
    u32 head[SPATIAL_SIZE];
    u32 value[SPATIAL_ENTRIES];
    u32 next[SPATIAL_ENTRIES];

    int count;
} SpatialTable;

typedef struct WorldSpatial
{
    SpatialTable staticWorld;
    SpatialTable dynamicUnits;
} WorldSpatial;

typedef struct
{
    vec3s a, b, c;
    vec3s normal;
} CollisionTri;

typedef struct
{
    uint32_t triIndices[MAX_TRIS_PER_CELL];
    uint32_t count;
} WorldCell;

typedef struct
{
    CollisionTri *tris;
    uint32_t triCount;
    WorldCell cells[WORLD_TABLE_SIZE];
} WorldCollider;

typedef struct
{
    int entityIds[MAX_ENTITIES_PER_CELL];
    int count;
} SpatialCell;

typedef struct
{
    SpatialCell cells[TABLE_SIZE];
} SpatialHash;

int GetHash(vec3s pos);
void BuildSpatialHash(World *world, SpatialHash *grid);
void ResolveCollisionsSpatial(World *world, SpatialHash *grid);
void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform);
void ResolveSphereMesh(CompBody *sphereBody, CompXform *sphereXform, CompModel *meshModel, CompXform *meshXform);
void ResolveSphereTriangle(CompBody *sphereBody, vec3s *localPos, vec3s a, vec3s b, vec3s c);
void ResolveWorldMeshSimple(CompBody *sphereBody, CompXform *sphereXform, CompXform *meshXform, SolModel *model);
void ResolveWorldCollisions(World *world);
void SimpleFloor(CompXform *xform, CompBody *body, float dt);