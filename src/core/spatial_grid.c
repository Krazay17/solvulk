#include "spatial_grid.h"
#include "sol_math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

SpatialGrid *SpatialGrid_Create(float cellSize)
{
    SpatialGrid *grid = (SpatialGrid *)malloc(sizeof(SpatialGrid));
    if (grid)
    {
        SpatialGrid_Init(grid, cellSize);
    }
    return grid;
}

void SpatialGrid_Init(SpatialGrid *grid, float cellSize)
{
    memset(grid, 0, sizeof(SpatialGrid));
    grid->cellSize = cellSize;
    if (cellSize > 0.0f)
    {
        grid->invCellSize = 1.0f / cellSize;
        grid->invCellSizeVec =
            (vec4s){.x = grid->invCellSize, .y = grid->invCellSize, .z = grid->invCellSize, .w = 0.0f};
    }
}

void SpatialGrid_Deinit(SpatialGrid *grid)
{
    if (!grid)
        return;

    if (grid->memory_block)
    {
        free(grid->memory_block);
    }
    memset(grid, 0, sizeof(SpatialGrid));
}

void SpatialGrid_Destroy(SpatialGrid *grid)
{
    if (!grid)
        return;

    SpatialGrid_Deinit(grid);
    free(grid);
}

void SpatialGrid_Clear(SpatialGrid *grid)
{
    if (!grid->cells || grid->totalCells == 0)
        return;

    memset(grid->cells, 0, grid->totalCells * sizeof(GridCell));
}

void SpatialGrid_BuildFromAABBs(SpatialGrid *grid, const SpatialAABB *boxes, uint32_t count)
{
    if (!grid || !boxes || count == 0 || grid->cellSize <= 0.0f)
        return;

    grid->item_count = count;

    if (grid->invCellSize == 0.0f)
    {
        grid->invCellSize = 1.0f / grid->cellSize;
        grid->invCellSizeVec =
            (vec4s){.x = grid->invCellSize, .y = grid->invCellSize, .z = grid->invCellSize, .w = 0.0f};
    }

    // PASS 0: Calculate global world bounds
    vec3s meshMin = boxes[0].min;
    vec3s meshMax = boxes[0].max;

    for (uint32_t i = 1; i < count; i++)
    {
        meshMin = glms_vec3_minv(meshMin, boxes[i].min);
        meshMax = glms_vec3_maxv(meshMax, boxes[i].max);
    }

    grid->min = (vec4s){.x = meshMin.x - 0.1f, .y = meshMin.y - 0.1f, .z = meshMin.z - 0.1f, .w = 0.0f};
    grid->max = (vec4s){.x = meshMax.x + 0.1f, .y = meshMax.y + 0.1f, .z = meshMax.z + 0.1f, .w = 0.0f};

    int gx = (int)ceilf((grid->max.x - grid->min.x) * grid->invCellSize);
    int gy = (int)ceilf((grid->max.y - grid->min.y) * grid->invCellSize);
    int gz = (int)ceilf((grid->max.z - grid->min.z) * grid->invCellSize);

    // TODO check if grid cap is wrong?
    grid->dims.x = gx <= 0 ? 1 : gx > 128 ? 128 : gx;
    grid->dims.y = gy <= 0 ? 1 : gy > 128 ? 128 : gy;
    grid->dims.z = gz <= 0 ? 1 : gz > 128 ? 128 : gz;

    grid->totalCells = (uint32_t)(grid->dims.x * grid->dims.y * grid->dims.z);

    // Prepare memory_block for cells pass
    size_t cellsSizeBytes = grid->totalCells * sizeof(GridCell);

    void *new_block = realloc(grid->memory_block, cellsSizeBytes);
    if (!new_block)
        return;

    grid->memory_block = new_block;
    grid->cells        = (GridCell *)grid->memory_block;
    grid->index_buffer = NULL; // Attached after reference counting

    memset(grid->cells, 0, cellsSizeBytes);

    // PASS 1: Count overlap references directly in grid->cells[cellIdx].count
    uint32_t totalReferences = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        ivec3s minCell = SpatialGrid_WorldToCell(grid, boxes[i].min);
        ivec3s maxCell = SpatialGrid_WorldToCell(grid, boxes[i].max);

        int minX = clampi(minCell.x, 0, grid->dims.x - 1);
        int maxX = clampi(maxCell.x, 0, grid->dims.x - 1);
        int minY = clampi(minCell.y, 0, grid->dims.y - 1);
        int maxY = clampi(maxCell.y, 0, grid->dims.y - 1);
        int minZ = clampi(minCell.z, 0, grid->dims.z - 1);
        int maxZ = clampi(maxCell.z, 0, grid->dims.z - 1);

        for (int z = minZ; z <= maxZ; z++)
        {
            for (int y = minY; y <= maxY; y++)
            {
                for (int x = minX; x <= maxX; x++)
                {
                    uint32_t cellIdx = SpatialGrid_GetCellIndex(grid, x, y, z);
                    grid->cells[cellIdx].count++;
                    totalReferences++;
                }
            }
        }
    }

    // Expand memory_block to hold contiguous cells + index_buffer
    size_t indexSizeBytes = totalReferences * sizeof(uint32_t);
    size_t totalBlockSize = cellsSizeBytes + indexSizeBytes;

    new_block = realloc(grid->memory_block, totalBlockSize);
    if (!new_block)
        return;

    grid->memory_block = new_block;
    grid->cells        = (GridCell *)grid->memory_block;
    grid->index_buffer = (uint32_t *)((uint8_t *)grid->memory_block + cellsSizeBytes);

    // PASS 2: Prefix sum for offsets & reset cell counts
    uint32_t currentOffset = 0;
    for (uint32_t i = 0; i < grid->totalCells; i++)
    {
        uint32_t cellCount    = grid->cells[i].count;
        grid->cells[i].offset = currentOffset;
        grid->cells[i].count  = 0;
        currentOffset += cellCount;
    }

    // PASS 3: Insert Entity / AABB IDs
    for (uint32_t i = 0; i < count; i++)
    {
        ivec3s minCell = SpatialGrid_WorldToCell(grid, boxes[i].min);
        ivec3s maxCell = SpatialGrid_WorldToCell(grid, boxes[i].max);

        int minX = clampi(minCell.x, 0, grid->dims.x - 1);
        int maxX = clampi(maxCell.x, 0, grid->dims.x - 1);
        int minY = clampi(minCell.y, 0, grid->dims.y - 1);
        int maxY = clampi(maxCell.y, 0, grid->dims.y - 1);
        int minZ = clampi(minCell.z, 0, grid->dims.z - 1);
        int maxZ = clampi(maxCell.z, 0, grid->dims.z - 1);

        for (int z = minZ; z <= maxZ; z++)
        {
            for (int y = minY; y <= maxY; y++)
            {
                for (int x = minX; x <= maxX; x++)
                {
                    uint32_t  cellIdx = SpatialGrid_GetCellIndex(grid, x, y, z);
                    GridCell *cell    = &grid->cells[cellIdx];

                    grid->index_buffer[cell->offset + cell->count] = boxes[i].id;
                    cell->count++;
                }
            }
        }
    }
}

typedef struct
{
    ivec3s minCell, maxCell;
} GridCellRange;

void SpatialGrid_BuildFromTris(SpatialGrid *grid, const SolTri *tris, uint32_t triCount)
{
    if (!grid || !tris || triCount == 0 || grid->cellSize <= 0.0f)
        return;

    grid->item_count = triCount;
    SpatialGrid_EnsureInvCellSize(grid);

    // PASS 0: World AABB bounds
    vec3s meshMin = tris[0].a, meshMax = tris[0].a;
    for (uint32_t i = 1; i < triCount; i++)
    {
        meshMin = glms_vec3_minv(meshMin, glms_vec3_minv(tris[i].a, glms_vec3_minv(tris[i].b, tris[i].c)));
        meshMax = glms_vec3_maxv(meshMax, glms_vec3_maxv(tris[i].a, glms_vec3_maxv(tris[i].b, tris[i].c)));
    }

    grid->min = (vec4s){meshMin.x - 0.1f, meshMin.y - 0.1f, meshMin.z - 0.1f, 0.0f};
    grid->max = (vec4s){meshMax.x + 0.1f, meshMax.y + 0.1f, meshMax.z + 0.1f, 0.0f};

    int gx           = (int)ceilf((grid->max.x - grid->min.x) * grid->invCellSize);
    int gy           = (int)ceilf((grid->max.y - grid->min.y) * grid->invCellSize);
    int gz           = (int)ceilf((grid->max.z - grid->min.z) * grid->invCellSize);
    grid->dims.x     = gx <= 0 ? 1 : gx;
    grid->dims.y     = gy <= 0 ? 1 : gy;
    grid->dims.z     = gz <= 0 ? 1 : gz;
    grid->totalCells = (uint32_t)(grid->dims.x * grid->dims.y * grid->dims.z);

    size_t cellsSizeBytes = grid->totalCells * sizeof(GridCell);
    void  *new_block      = realloc(grid->memory_block, cellsSizeBytes);
    if (!new_block)
        return;
    grid->memory_block = new_block;
    grid->cells        = (GridCell *)grid->memory_block;
    grid->index_buffer = NULL;
    memset(grid->cells, 0, cellsSizeBytes);

    // Cache each triangle's cell range ONCE -- reused in PASS 3 instead of
    // recomputing 2x glms_vec3_minv/maxv + 2x SIMD WorldToCell per triangle.
    GridCellRange *ranges = (GridCellRange *)malloc(triCount * sizeof(GridCellRange));
    if (!ranges)
        return;

    // PASS 1: Count references
    uint32_t totalReferences = 0;
    for (uint32_t i = 0; i < triCount; i++)
    {
        vec3s tMin = glms_vec3_minv(tris[i].a, glms_vec3_minv(tris[i].b, tris[i].c));
        vec3s tMax = glms_vec3_maxv(tris[i].a, glms_vec3_maxv(tris[i].b, tris[i].c));

        ivec3s minCell    = SpatialGrid_WorldToCell(grid, tMin);
        ivec3s maxCell    = SpatialGrid_WorldToCell(grid, tMax);
        minCell.x         = clampi(minCell.x, 0, grid->dims.x - 1);
        maxCell.x         = clampi(maxCell.x, 0, grid->dims.x - 1);
        minCell.y         = clampi(minCell.y, 0, grid->dims.y - 1);
        maxCell.y         = clampi(maxCell.y, 0, grid->dims.y - 1);
        minCell.z         = clampi(minCell.z, 0, grid->dims.z - 1);
        maxCell.z         = clampi(maxCell.z, 0, grid->dims.z - 1);
        ranges[i].minCell = minCell;
        ranges[i].maxCell = maxCell;

        for (int z = minCell.z; z <= maxCell.z; z++)
            for (int y = minCell.y; y <= maxCell.y; y++)
                for (int x = minCell.x; x <= maxCell.x; x++)
                {
                    grid->cells[SpatialGrid_GetCellIndex(grid, x, y, z)].count++;
                    totalReferences++;
                }
    }

    size_t indexSizeBytes = totalReferences * sizeof(uint32_t);
    new_block             = realloc(grid->memory_block, cellsSizeBytes + indexSizeBytes);
    if (!new_block)
    {
        free(ranges);
        return;
    }
    grid->memory_block = new_block;
    grid->cells        = (GridCell *)grid->memory_block;
    grid->index_buffer = (uint32_t *)((uint8_t *)grid->memory_block + cellsSizeBytes);

    // PASS 2: Prefix sum
    uint32_t currentOffset = 0;
    for (uint32_t i = 0; i < grid->totalCells; i++)
    {
        uint32_t c            = grid->cells[i].count;
        grid->cells[i].offset = currentOffset;
        grid->cells[i].count  = 0;
        currentOffset += c;
    }

    // PASS 3: Populate index buffer -- uses cached ranges, no recomputation
    for (uint32_t i = 0; i < triCount; i++)
    {
        ivec3s minCell = ranges[i].minCell, maxCell = ranges[i].maxCell;
        for (int z = minCell.z; z <= maxCell.z; z++)
            for (int y = minCell.y; y <= maxCell.y; y++)
                for (int x = minCell.x; x <= maxCell.x; x++)
                {
                    uint32_t  cellIdx                              = SpatialGrid_GetCellIndex(grid, x, y, z);
                    GridCell *cell                                 = &grid->cells[cellIdx];
                    grid->index_buffer[cell->offset + cell->count] = i;
                    cell->count++;
                }
    }

    free(ranges);
}