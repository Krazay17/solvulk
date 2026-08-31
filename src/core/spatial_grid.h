#pragma once
#include "sol/types.h"

#define STAGE_TRI_INDEX_BITS 20
#define STAGE_TRI_INDEX_MASK ((1U << STAGE_TRI_INDEX_BITS) - 1) // 0x000FFFFF

typedef struct
{
    uint32_t offset; // Index into global index_buffer
    uint32_t count;  // Number of triangles in this cell
} GridCell;

typedef struct
{
    vec3s    min, max;
    float    cellSize, invCellSize;
    uint32_t gridX, gridY, gridZ;
    uint32_t totalCells;

    void     *memory_block; // <--- The ONLY heap ownership handle
    GridCell *cells;        // View into memory_block
    uint32_t *index_buffer; // View into memory_block
    uint32_t  item_count;
} SpatialGrid;

// World pos -> cell coordinate
static inline ivec3s SpatialGrid_WorldToCell(const SpatialGrid *grid, vec3s pos)
{
    return (ivec3s){.x = (int)floorf((pos.x - grid->min.x) * grid->invCellSize),
                    .y = (int)floorf((pos.y - grid->min.y) * grid->invCellSize),
                    .z = (int)floorf((pos.z - grid->min.z) * grid->invCellSize)};
}

// Cell coordinate -> flat array index
static inline uint32_t SpatialGrid_GetCellIndex(const SpatialGrid *grid, int x, int y, int z)
{
    // Clamp to grid limits to prevent out-of-bounds reads
    x = x < 0 ? 0 : (x >= (int)grid->gridX ? (int)grid->gridX - 1 : x);
    y = y < 0 ? 0 : (y >= (int)grid->gridY ? (int)grid->gridY - 1 : y);
    z = z < 0 ? 0 : (z >= (int)grid->gridZ ? (int)grid->gridZ - 1 : z);

    return (uint32_t)(x + y * grid->gridX + z * grid->gridX * grid->gridY);
}

static inline uint32_t Pack_StageTri(uint32_t entityId, uint32_t triIndex)
{
    return (entityId << STAGE_TRI_INDEX_BITS) | (triIndex & STAGE_TRI_INDEX_MASK);
}

static inline void Unpack_StageTri(uint32_t packedData, uint32_t *outEntityId, uint32_t *outTriIndex)
{
    *outEntityId = packedData >> STAGE_TRI_INDEX_BITS;
    *outTriIndex = packedData & STAGE_TRI_INDEX_MASK;
}

SpatialGrid *SpatialGrid_Create(float cellSize);
void         SpatialGrid_Init(SpatialGrid *grid, float cellSize);
void         SpatialGrid_Destroy(SpatialGrid *grid);
void         SpatialGrid_BuildFromTris(SpatialGrid *grid, const SolTri *tris, uint32_t triCount, int id);
void         SpatialGrid_Clear(SpatialGrid *grid);
