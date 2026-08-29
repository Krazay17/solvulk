/*
 * File: spatial.h
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
    float cellSize    = table->cellSize;
    int   mask        = table->size - 1;

    int x0 = fast_floor(min.x * invCellSize);
    int x1 = fast_floor(max.x * invCellSize);
    int y0 = fast_floor(min.y * invCellSize);
    int y1 = fast_floor(max.y * invCellSize);
    int z0 = fast_floor(min.z * invCellSize);
    int z1 = fast_floor(max.z * invCellSize);

    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++)
            for (int z = z0; z <= z1; z++)
                SpatialTable_Insert(table, hash_coords(x, y, z) & mask, id);
}

static inline void Spatial_Hash_Tris(SpatialTable *table, SolTri *tris, int count, int id)
{
    u32   size     = table->size;
    float cellSize = table->cellSize;
    for (int i = 0; i < count; i++)
    {
        float minX = fminf(tris[i].a.x, fminf(tris[i].b.x, tris[i].c.x));
        float maxX = fmaxf(tris[i].a.x, fmaxf(tris[i].b.x, tris[i].c.x));
        float minY = fminf(tris[i].a.y, fminf(tris[i].b.y, tris[i].c.y));
        float maxY = fmaxf(tris[i].a.y, fmaxf(tris[i].b.y, tris[i].c.y));
        float minZ = fminf(tris[i].a.z, fminf(tris[i].b.z, tris[i].c.z));
        float maxZ = fmaxf(tris[i].a.z, fmaxf(tris[i].b.z, tris[i].c.z));

        int x0 = (int)floorf(minX / cellSize);
        int x1 = (int)floorf(maxX / cellSize);
        int y0 = (int)floorf(minY / cellSize);
        int y1 = (int)floorf(maxY / cellSize);
        int z0 = (int)floorf(minZ / cellSize);
        int z1 = (int)floorf(maxZ / cellSize);

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++)
                {
                    u32 hash = (u32)((u64)hash_coords(x, y, z) * (u64)size >> 32);
                    SpatialTable_Insert(table, hash, id);
                }
    }
}

// static inline void Spatial_Hash_Tri(SpatialTable *table, const SolTri *tri, int id)
// {
//     u32 mask = table->size - 1;
//     float invCellSize = table->invCellSize;
//     float cellSize    = table->cellSize;
//     float minX        = fminf(tri->a.x, fminf(tri->b.x, tri->c.x));
//     float maxX        = fmaxf(tri->a.x, fmaxf(tri->b.x, tri->c.x));
//     float minY        = fminf(tri->a.y, fminf(tri->b.y, tri->c.y));
//     float maxY        = fmaxf(tri->a.y, fmaxf(tri->b.y, tri->c.y));
//     float minZ        = fminf(tri->a.z, fminf(tri->b.z, tri->c.z));
//     float maxZ        = fmaxf(tri->a.z, fmaxf(tri->b.z, tri->c.z));

//     int x0 = fast_floor(minX * invCellSize);
//     int x1 = fast_floor(maxX * invCellSize);
//     int y0 = fast_floor(minY * invCellSize);
//     int y1 = fast_floor(maxY * invCellSize);
//     int z0 = fast_floor(minZ * invCellSize);
//     int z1 = fast_floor(maxZ * invCellSize);

//     for (int x = x0; x <= x1; x++)
//         for (int y = y0; y <= y1; y++)
//             for (int z = z0; z <= z1; z++)
//                 SpatialTable_Insert(table, hash_coords(x, y, z) & mask, id);
// }
static inline int fast_floor_int(float x)
{
    int i = (int)x;
    return i - (x < (float)i);
}

static inline void Spatial_Hash_Tri(SpatialTable *table, const SolTri *tri, u32 value)
{
    const float invCellSize = table->invCellSize;
    const u32   mask        = table->size - 1;

    // 1. Scalar Min/Max using ternary operators (compiles to CMOV / MINSS / MAXSS)
    float minX = tri->a.x < tri->b.x ? (tri->a.x < tri->c.x ? tri->a.x : tri->c.x) : (tri->b.x < tri->c.x ? tri->b.x : tri->c.x);
    float maxX = tri->a.x > tri->b.x ? (tri->a.x > tri->c.x ? tri->a.x : tri->c.x) : (tri->b.x > tri->c.x ? tri->b.x : tri->c.x);

    float minY = tri->a.y < tri->b.y ? (tri->a.y < tri->c.y ? tri->a.y : tri->c.y) : (tri->b.y < tri->c.y ? tri->b.y : tri->c.y);
    float maxY = tri->a.y > tri->b.y ? (tri->a.y > tri->c.y ? tri->a.y : tri->c.y) : (tri->b.y > tri->c.y ? tri->b.y : tri->c.y);

    float minZ = tri->a.z < tri->b.z ? (tri->a.z < tri->c.z ? tri->a.z : tri->c.z) : (tri->b.z < tri->c.z ? tri->b.z : tri->c.z);
    float maxZ = tri->a.z > tri->b.z ? (tri->a.z > tri->c.z ? tri->a.z : tri->c.z) : (tri->b.z > tri->c.z ? tri->b.z : tri->c.z);

    // 2. Scale & Fast Floor
    int x0 = fast_floor_int(minX * invCellSize);
    int x1 = fast_floor_int(maxX * invCellSize);
    int y0 = fast_floor_int(minY * invCellSize);
    int y1 = fast_floor_int(maxY * invCellSize);
    int z0 = fast_floor_int(minZ * invCellSize);
    int z1 = fast_floor_int(maxZ * invCellSize);

    // 3. Early single-cell bypass (Fast Path for >90% of triangles)
    if (x0 == x1 && y0 == y1 && z0 == z1)
    {
        u32 bucketKey = hash_coords(x0, y0, z0) & mask;
        SpatialTable_Insert(table, bucketKey, value);
        return;
    }

    // 4. Multi-cell loop fallback
    for (int x = x0; x <= x1; ++x)
    {
        for (int y = y0; y <= y1; ++y)
        {
            for (int z = z0; z <= z1; ++z)
            {
                u32 bucketKey = hash_coords(x, y, z) & mask;
                SpatialTable_Insert(table, bucketKey, value);
            }
        }
    }
}