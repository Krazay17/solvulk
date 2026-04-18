// internal_physx.h
#pragma once

#include "internal_types.h"
#include "sol/types.h"

typedef struct CompBody CompBody;
typedef struct CompXform CompXform;
// 131072
// 131101
#define SOL_TIMESTEP 1.0 / 60.0
#define SOL_PHYS_COLLISION_SKIN 0.01f

#define SPATIAL_NULL 0xFFFFFFFF
#define SPATIAL_CELL_SIZE 2.0f

#define SPATIAL_DYNAMIC_SIZE (1 << 14)
#define SPATIAL_DYNAMIC_ENTRIES 65536

#define SPATIAL_STATIC_SIZE (1 << 22)
#define SPATIAL_STATIC_ENTRIES 0x0FFFFFFF

typedef SolCollision (*ResolveShapeTri)(CompBody *body, CompXform *xform, CollisionTri *tri);

typedef struct
{
    u8 substeps;
    float sub_dt;
} SubstepData;

u32 HashCoords(int x, int y, int z, u32 mask);

vec3s ClosestPointOnTriangle(vec3s p, vec3s a, vec3s b, vec3s c);

void SpatialTable_Init(SpatialTable *table, u32 buckets, u32 capacity);
void SpatialTable_Clear(SpatialTable *table);
void SpatialTable_Free(SpatialTable *table);
void SpatialTable_Insert(SpatialTable *table, u32 hash, u32 value);
void SpatialTable_Compact(SpatialTable *table);

SolCollision ResolveSphereTriangle(CompBody *sphereBody, CompXform *xform, CollisionTri *tri);
SolCollision ResolveCapsuleTriangle(CompBody *sphereBody, CompXform *xform, CollisionTri *tri);

void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform);
void SimpleFloor(CompXform *xform, CompBody *body, float dt);

SpatialCell GetSpatialCell(vec3s pos);
void Sol_Spatial_Build_Dynamic(World *world);

void resolve_position_only(CompBody *aBody, CompXform *aXform,
                         CompBody *bBody, CompXform *bXform);

void resolve_velocity_only(CompBody *aBody, CompXform *aXform,
                         CompBody *bBody, CompXform *bXform);

void StaticGrid_Build(StaticGrid *grid, WorldSpatial *ws,
                      vec3s worldMin, vec3s worldMax, float cellSize);

void static_collisions_grid(CompBody *body, CompXform *xform, SubstepData substep_data, StaticGrid *grid, WorldSpatial *ws);

SubstepData substep_get(CompBody *body, float fdt);
void dynamic_collisions_hashed(World *world, int id, CompBody *body, CompXform *xform);

static inline u32 HashCoordsRaw(int x, int y, int z)
{
    return ((unsigned int)x * 73856093) ^
           ((unsigned int)y * 19349663) ^
           ((unsigned int)z * 83492791);
}

static inline u32 StaticGrid_CellIndex(StaticGrid *grid, vec3s pos)
{
    int x = (int)floorf((pos.x - grid->worldMin.x) / grid->cellSize);
    int y = (int)floorf((pos.y - grid->worldMin.y) / grid->cellSize);
    int z = (int)floorf((pos.z - grid->worldMin.z) / grid->cellSize);

    x = x < 0 ? 0 : (x >= grid->gridX ? grid->gridX - 1 : x);
    y = y < 0 ? 0 : (y >= grid->gridY ? grid->gridY - 1 : y);
    z = z < 0 ? 0 : (z >= grid->gridZ ? grid->gridZ - 1 : z);

    return x + y * grid->gridX + z * grid->gridX * grid->gridY;
}