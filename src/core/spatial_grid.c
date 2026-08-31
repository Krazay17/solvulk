#include "spatial_grid.h"
#include "sol_math.h"

SpatialGrid *SpatialGrid_Create(float cellSize)
{
    SpatialGrid *grid = malloc(sizeof(SpatialGrid));
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
    if (cellSize != 0.0f)
        grid->invCellSize = 1.0f / cellSize;
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
    if (grid->memory_block)
        free(grid->memory_block);
    free(grid);
}

void SpatialGrid_Clear(SpatialGrid *grid)
{
    if (!grid->cells || grid->totalCells == 0)
        return;

    memset(grid->cells, 0, grid->totalCells * sizeof(GridCell));
}

void SpatialGrid_BuildFromTris(SpatialGrid *grid, const SolTri *tris, uint32_t triCount, int id)
{
    float cellSize    = grid->cellSize;
    float invCellSize = grid->invCellSize;
    if (!grid || !tris || triCount == 0 || cellSize <= 0.0f)
        return;

    grid->item_count += triCount;

    // -------------------------------------------------------------
    // PASS 0: World AABB Bounds from SolTri vertices directly
    // -------------------------------------------------------------
    vec3s meshMin = tris[0].a;
    vec3s meshMax = tris[0].a;

    for (uint32_t i = 0; i < triCount; i++)
    {
        meshMin = glms_vec3_minv(meshMin, glms_vec3_minv(tris[i].a, glms_vec3_minv(tris[i].b, tris[i].c)));
        meshMax = glms_vec3_maxv(meshMax, glms_vec3_maxv(tris[i].a, glms_vec3_maxv(tris[i].b, tris[i].c)));
    }

    grid->min = (vec3s){meshMin.x - 0.1f, meshMin.y - 0.1f, meshMin.z - 0.1f};
    grid->max = (vec3s){meshMax.x + 0.1f, meshMax.y + 0.1f, meshMax.z + 0.1f};

    grid->gridX = (uint32_t)ceilf((grid->max.x - grid->min.x) * invCellSize);
    grid->gridY = (uint32_t)ceilf((grid->max.y - grid->min.y) * invCellSize);
    grid->gridZ = (uint32_t)ceilf((grid->max.z - grid->min.z) * invCellSize);

    grid->gridX = grid->gridX == 0 ? 1 : grid->gridX;
    grid->gridY = grid->gridY == 0 ? 1 : grid->gridY;
    grid->gridZ = grid->gridZ == 0 ? 1 : grid->gridZ;

    grid->totalCells = grid->gridX * grid->gridY * grid->gridZ;

    uint32_t *tempCellCounts = calloc(grid->totalCells, sizeof(uint32_t));

    // -------------------------------------------------------------
    // PASS 1: Count triangle references per cell
    // -------------------------------------------------------------
    uint32_t totalReferences = 0;
    for (uint32_t i = 0; i < triCount; i++)
    {
        vec3s v0 = tris[i].a, v1 = tris[i].b, v2 = tris[i].c;

        vec3s tMin = glms_vec3_minv(v0, glms_vec3_minv(v1, v2));
        vec3s tMax = glms_vec3_maxv(v0, glms_vec3_maxv(v1, v2));

        ivec3s minCell = SpatialGrid_WorldToCell(grid, tMin);
        ivec3s maxCell = SpatialGrid_WorldToCell(grid, tMax);

        for (int z = minCell.z; z <= maxCell.z; z++)
        {
            for (int y = minCell.y; y <= maxCell.y; y++)
            {
                for (int x = minCell.x; x <= maxCell.x; x++)
                {
                    uint32_t cellIdx = SpatialGrid_GetCellIndex(grid, x, y, z);
                    tempCellCounts[cellIdx]++;
                    totalReferences++;
                }
            }
        }
    }

    // -------------------------------------------------------------
    // Single Contiguous Allocation
    // -------------------------------------------------------------
    size_t cellsSizeBytes = grid->totalCells * sizeof(GridCell);
    size_t indexSizeBytes = totalReferences * sizeof(uint32_t);
    size_t totalBlockSize = cellsSizeBytes + indexSizeBytes;

    if (grid->memory_block)
    {
        free(grid->memory_block);
    }

    grid->memory_block = malloc(totalBlockSize);
    grid->cells        = (GridCell *)grid->memory_block;
    grid->index_buffer = (uint32_t *)((uint8_t *)grid->memory_block + cellsSizeBytes);

    // -------------------------------------------------------------
    // PASS 2: Prefix sum for offsets
    // -------------------------------------------------------------
    uint32_t currentOffset = 0;
    for (uint32_t i = 0; i < grid->totalCells; i++)
    {
        grid->cells[i].offset = currentOffset;
        grid->cells[i].count  = 0;
        currentOffset += tempCellCounts[i];
    }

    free(tempCellCounts);

    // -------------------------------------------------------------
    // PASS 3: Populate continuous index buffer
    // -------------------------------------------------------------
    for (uint32_t i = 0; i < triCount; i++)
    {
        vec3s v0 = tris[i].a, v1 = tris[i].b, v2 = tris[i].c;

        vec3s tMin = glms_vec3_minv(v0, glms_vec3_minv(v1, v2));
        vec3s tMax = glms_vec3_maxv(v0, glms_vec3_maxv(v1, v2));

        ivec3s minCell = SpatialGrid_WorldToCell(grid, tMin);
        ivec3s maxCell = SpatialGrid_WorldToCell(grid, tMax);

        for (int z = minCell.z; z <= maxCell.z; z++)
        {
            for (int y = minCell.y; y <= maxCell.y; y++)
            {
                for (int x = minCell.x; x <= maxCell.x; x++)
                {
                    uint32_t  cellIdx = SpatialGrid_GetCellIndex(grid, x, y, z);
                    GridCell *cell    = &grid->cells[cellIdx];

                    grid->index_buffer[cell->offset + cell->count] = Pack_StageTri(id, i);
                    cell->count++;
                }
            }
        }
    }
}