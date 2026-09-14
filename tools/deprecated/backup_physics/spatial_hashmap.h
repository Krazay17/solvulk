/*
 * File: spatial_hashmap.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-28
 *
 */
#pragma once
#include "sol/base.h"

#define SPATIAL_NULL 0xFFFFFFFFU

typedef struct SpatialCell
{
    int ix, iy, iz;
    u32 neighborHashes[27];
} SpatialCell;

typedef struct
{
    vec3s min;
    vec3s max;
} BoundingBox;

typedef struct SpatialTable
{
    u32  *head;
    u32  *value;
    u32  *next;
    u32   capacity;
    u32   size;
    u32   count;
    float invCellSize;
    float cellSize;
} SpatialTable;

static inline u32 hash_coords(int x, int y, int z)
{
    return ((unsigned int)x * 73856093) ^ ((unsigned int)y * 19349663) ^ ((unsigned int)z * 83492791);
}

static inline u32 cell_index(vec3s pos, vec3s grid_min, vec3s grid_dims, float cell_size)
{
    int x = (int)floorf((pos.x - grid_min.x) / cell_size);
    int y = (int)floorf((pos.y - grid_min.y) / cell_size);
    int z = (int)floorf((pos.z - grid_min.z) / cell_size);

    x = x < 0 ? 0 : (int)(x >= grid_dims.x ? grid_dims.x - 1 : x);
    y = y < 0 ? 0 : (int)(y >= grid_dims.y ? grid_dims.y - 1 : y);
    z = z < 0 ? 0 : (int)(z >= grid_dims.z ? grid_dims.z - 1 : z);

    return (u32)(x + y * grid_dims.x + z * grid_dims.x * grid_dims.y);
}

static inline void SpatialTable_Clear(SpatialTable *table)
{
    memset(table->head, 0xFF, sizeof(u32) * table->size);
    table->count = 0;
}

static inline u32 SpatialTable_GetEntry(const SpatialTable *table, u32 hash)
{
    return table->head[hash & table->size - 1];
}

// make bucket size and cap power of 2 ex: (1 << 12)
static inline void SpatialTable_Init(SpatialTable *table, u32 buckets, u32 capacity, float cellSize)
{

    table->head        = (uint32_t *)malloc(sizeof(u32) * buckets);
    table->value       = (uint32_t *)malloc(sizeof(u32) * capacity);
    table->next        = (uint32_t *)malloc(sizeof(u32) * capacity);
    table->size        = buckets;
    table->capacity    = capacity;
    table->cellSize    = cellSize;
    table->invCellSize = 1.0f / table->cellSize;
    SpatialTable_Clear(table);
}

static inline void SpatialTable_Free(SpatialTable *table)
{
    free(table->head);
    free(table->value);
    free(table->next);
}

static inline void SpatialTable_Insert(SpatialTable *table, u32 hash, u32 value)
{
    assert(table->count < table->capacity && "SpatialTable count larger than cap");

    u32 idx           = table->count++;
    table->value[idx] = value;
    table->next[idx]  = table->head[hash];
    table->head[hash] = idx;
}

static inline SpatialCell Spatial_Cell_GetNeighbors(vec3s pos, float cellSize)
{
    SpatialCell cell;
    cell.ix = (int)floorf(pos.x / cellSize);
    cell.iy = (int)floorf(pos.y / cellSize);
    cell.iz = (int)floorf(pos.z / cellSize);

    int n = 0;
    for (int ox = -1; ox <= 1; ox++)
        for (int oy = -1; oy <= 1; oy++)
            for (int oz = -1; oz <= 1; oz++)
                cell.neighborHashes[n++] = hash_coords(cell.ix + ox, cell.iy + oy, cell.iz + oz);
    return cell;
}

static inline u32 Spatial_Cell_Get(vec3s pos, float cellSize)
{
    int x = (int)floorf(pos.x / cellSize);
    int y = (int)floorf(pos.y / cellSize);
    int z = (int)floorf(pos.z / cellSize);
    return hash_coords(x, y, z);
}

static inline void Spatial_FloorHashInsert(SpatialTable *table, vec3s min, vec3s max, int id)
{
    float invCellSize = table->invCellSize;
    int   mask        = table->size - 1;
    int   x0          = (int)floorf(min.x * invCellSize);
    int   x1          = (int)floorf(max.x * invCellSize);
    int   y0          = (int)floorf(min.y * invCellSize);
    int   y1          = (int)floorf(max.y * invCellSize);
    int   z0          = (int)floorf(min.z * invCellSize);
    int   z1          = (int)floorf(max.z * invCellSize);

    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++)
            for (int z = z0; z <= z1; z++)
                SpatialTable_Insert(table, hash_coords(x, y, z) & mask, id);
}