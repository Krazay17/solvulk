// internal_physx.h
#pragma once

#include "internal_types.h"
#include "sol/types.h"

typedef struct CompBody CompBody;
typedef struct CompXform CompXform;

#define SOL_TIMESTEP 1.0 / 60.0
#define SOL_PHYS_COLLISION_SKIN 0.01f

#define SPATIAL_NULL 0xFFFFFFFF

#define SPATIAL_DYNAMIC_CELL_SIZE 4.0f
#define SPATIAL_DYNAMIC_SIZE (1 << 14)
#define SPATIAL_DYNAMIC_ENTRIES 0xFFFF

#define SPATIAL_STATIC_CELL_SIZE 2.0f
#define SPATIAL_STATIC_SIZE (1 << 22)
#define SPATIAL_STATIC_ENTRIES 0xFFFFFFF

typedef SolCollision (*ResolveShapeTri)(CompBody *body, CompXform *xform, SolTri *tri);

typedef struct
{
    u8 substeps;
    float sub_dt;
} SubstepData;

// Per-entity substep count based on speed
SubstepData substep_get(CompBody *body, float fdt);

SpatialCell spatial_cell_get(vec3s pos, float cell_size);
void SpatialTable_Init(SpatialTable *table, u32 buckets, u32 capacity);
void SpatialTable_Clear(SpatialTable *table);
void SpatialTable_Free(SpatialTable *table);
void SpatialTable_Insert(SpatialTable *table, u32 hash, u32 value);
void SpatialTable_Compact(SpatialTable *table);

void spatial_static_add_model(WorldSpatial *ws, SolModel *model, CompXform *xform);

void physx_grid_build_static(WorldSpatial *ws, vec3s min, vec3s max, float cell_size);

void collisions_grid_static(CompBody *body, CompXform *xform, WorldSpatial *ws, float fdt);
void collisions_hash_dynamic(World *world, int id, CompBody *body, CompXform *xform);

SolCollision collide_y0(CompXform *xform, CompBody *body);
SolCollision collide_sphere_tri(CompBody *body, CompXform *xform, SolTri *tri);
SolCollision collide_capsule_tri(CompBody *body, CompXform *xform, SolTri *tri);
SolCollision collide_box_tri(CompBody *body, CompXform *xform, SolTri *tri);

SolCollision collide_sphere_sphere(CompBody *aBody, CompXform *aXform,
                                   CompBody *bBody, CompXform *bXform);
SolCollision collide_sphere_rect(CompBody *aBody, CompXform *aXform,
                                 CompBody *bBody, CompXform *bXform);

static inline u32 hash_coords(int x, int y, int z)
{
    return ((unsigned int)x * 73856093) ^
           ((unsigned int)y * 19349663) ^
           ((unsigned int)z * 83492791);
}

static inline u32 cell_index(SpatialGrid *grid, vec3s pos)
{
    int x = (int)floorf((pos.x - grid->min.x) / grid->cellSize);
    int y = (int)floorf((pos.y - grid->min.y) / grid->cellSize);
    int z = (int)floorf((pos.z - grid->min.z) / grid->cellSize);

    x = x < 0 ? 0 : (x >= grid->gridX ? grid->gridX - 1 : x);
    y = y < 0 ? 0 : (y >= grid->gridY ? grid->gridY - 1 : y);
    z = z < 0 ? 0 : (z >= grid->gridZ ? grid->gridZ - 1 : z);

    return x + y * grid->gridX + z * grid->gridX * grid->gridY;
}