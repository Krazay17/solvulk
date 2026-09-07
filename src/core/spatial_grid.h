#pragma once
#include "sol/types.h"
#include <emmintrin.h>
#include <smmintrin.h> // For SSE4.1 _mm_floor_ps

typedef struct
{
    vec3s min, max;
    int   id;
} SpatialAABB;

typedef struct
{
    uint32_t offset; // Index into global index_buffer
    uint32_t count;  // Number of items/triangles in this cell
} GridCell;
#define SPATIAL_GRID_DEFAULT_MAX_CELLS_PER_AXIS 128

typedef struct
{
    vec4s min, max, invCellSizeVec;

    float baseCellSize;      // preferred/minimum resolution -- set once at Init, never changes
    float cellSize, invCellSize; // EFFECTIVE resolution for the current build; may grow
                                  // above baseCellSize if the tracked extent needs it
    int   maxCellsPerAxis;   // hard budget on dims.{x,y,z} -- cellSize scales to respect this

    ivec3s   dims;
    uint32_t totalCells;

    void     *memory_block;
    GridCell *cells;
    uint32_t *index_buffer;
    uint32_t  item_count;
} SpatialGrid;

void SpatialGrid_SetMaxCellsPerAxis(SpatialGrid *grid, int maxCellsPerAxis);

typedef struct
{
    ivec3s step;
    ivec3s cell;
    vec3s  tMax;
    vec3s  tDelta;
} GridDDA;


// World pos -> cell coordinate (SIMD Optimized)
static inline ivec3s SpatialGrid_WorldToCell(const SpatialGrid *grid, vec3s pos)
{
    // Load pos into a 128-bit SIMD register
    __m128 vpos = _mm_setr_ps(pos.x, pos.y, pos.z, 0.0f);

    // Load min and invCellSizeVec directly from grid struct
    __m128 vmin = _mm_loadu_ps((const float *)&grid->min);
    __m128 vinv = _mm_loadu_ps((const float *)&grid->invCellSizeVec);

    // Parallel Subtraction & Multiplication: (pos - min) * invCellSize
    __m128 vscaled = _mm_mul_ps(_mm_sub_ps(vpos, vmin), vinv);

    // Parallel Floor and Float-to-Int Conversion
#if defined(__SSE4_1__)
    __m128  vfloored = _mm_floor_ps(vscaled);
    __m128i vi       = _mm_cvttps_epi32(vfloored);
#else
    // Correct SSE2 fallback floor:
    __m128i vi   = _mm_cvttps_epi32(vscaled);
    __m128  v_i  = _mm_cvtepi32_ps(vi);
    __m128  mask = _mm_cmplt_ps(vscaled, v_i);
    vi           = _mm_add_epi32(vi, _mm_castps_si128(mask));
#endif

    // Unpack result into ivec3s
    int tmp[4];
    _mm_storeu_si128((__m128i *)tmp, vi);

    return (ivec3s){tmp[0], tmp[1], tmp[2]};
}

// Cell coordinate -> flat array index
static inline uint32_t SpatialGrid_GetCellIndex(const SpatialGrid *grid, int x, int y, int z)
{
    // Clamp to grid limits to prevent out-of-bounds reads
    x = x < 0 ? 0 : (x >= grid->dims.x ? grid->dims.x - 1 : x);
    y = y < 0 ? 0 : (y >= grid->dims.y ? grid->dims.y - 1 : y);
    z = z < 0 ? 0 : (z >= grid->dims.z ? grid->dims.z - 1 : z);

    return (uint32_t)(x + y * grid->dims.x + z * grid->dims.x * grid->dims.y);
}

static inline void SpatialGrid_EnsureInvCellSize(SpatialGrid *grid)
{
    if (grid->invCellSize == 0.0f && grid->cellSize > 0.0f)
    {
        grid->invCellSize    = 1.0f / grid->cellSize;
        grid->invCellSizeVec = (vec4s){grid->invCellSize, grid->invCellSize, grid->invCellSize, 0.0f};
    }
}

// dir MUST already be normalized.
static inline GridDDA GridDDA_Init(const SpatialGrid *grid, vec3s start, vec3s dir)
{
    GridDDA it;
    it.cell = SpatialGrid_WorldToCell(grid, start);

    it.step.x = (dir.x > 0.0f) ? 1 : (dir.x < 0.0f ? -1 : 0);
    it.step.y = (dir.y > 0.0f) ? 1 : (dir.y < 0.0f ? -1 : 0);
    it.step.z = (dir.z > 0.0f) ? 1 : (dir.z < 0.0f ? -1 : 0);

    const float cs = grid->cellSize; // <-- the actual grid cell size, not range/dims

    it.tDelta.x = (it.step.x != 0) ? cs * fabsf(1.0f / dir.x) : 1e30f;
    it.tDelta.y = (it.step.y != 0) ? cs * fabsf(1.0f / dir.y) : 1e30f;
    it.tDelta.z = (it.step.z != 0) ? cs * fabsf(1.0f / dir.z) : 1e30f;

    vec3s cellMin = {grid->min.x + (float)it.cell.x * cs, grid->min.y + (float)it.cell.y * cs,
                     grid->min.z + (float)it.cell.z * cs};

    float invDirX = (dir.x != 0.0f) ? 1.0f / dir.x : 0.0f;
    float invDirY = (dir.y != 0.0f) ? 1.0f / dir.y : 0.0f;
    float invDirZ = (dir.z != 0.0f) ? 1.0f / dir.z : 0.0f;

    it.tMax.x = (it.step.x > 0)   ? (cellMin.x + cs - start.x) * invDirX
                : (it.step.x < 0) ? (cellMin.x - start.x) * invDirX
                                  : 1e30f;
    it.tMax.y = (it.step.y > 0)   ? (cellMin.y + cs - start.y) * invDirY
                : (it.step.y < 0) ? (cellMin.y - start.y) * invDirY
                                  : 1e30f;
    it.tMax.z = (it.step.z > 0)   ? (cellMin.z + cs - start.z) * invDirZ
                : (it.step.z < 0) ? (cellMin.z - start.z) * invDirZ
                                  : 1e30f;

    it.tMax.x = fmaxf(it.tMax.x, 0.0f);
    it.tMax.y = fmaxf(it.tMax.y, 0.0f);
    it.tMax.z = fmaxf(it.tMax.z, 0.0f);

    return it;
}

static inline bool GridDDA_InBounds(const GridDDA *it, const SpatialGrid *grid)
{
    return it->cell.x >= 0 && it->cell.x < grid->dims.x && it->cell.y >= 0 && it->cell.y < grid->dims.y &&
           it->cell.z >= 0 && it->cell.z < grid->dims.z;
}

// Advances to the next cell; returns the ray distance at which it was entered.
static inline float GridDDA_Step(GridDDA *it)
{
    float t;
    if (it->tMax.x < it->tMax.y)
    {
        if (it->tMax.x < it->tMax.z) { t = it->tMax.x; it->cell.x += it->step.x; it->tMax.x += it->tDelta.x; }
        else                         { t = it->tMax.z; it->cell.z += it->step.z; it->tMax.z += it->tDelta.z; }
    }
    else
    {
        if (it->tMax.y < it->tMax.z) { t = it->tMax.y; it->cell.y += it->step.y; it->tMax.y += it->tDelta.y; }
        else                         { t = it->tMax.z; it->cell.z += it->step.z; it->tMax.z += it->tDelta.z; }
    }
    return t;
}

// Safe to use only where bounds are already guaranteed (e.g. inside a
// GridDDA_InBounds-gated loop). No clamping — that's the whole point.
static inline uint32_t SpatialGrid_GetCellIndexUnchecked(const SpatialGrid *grid, int x, int y, int z)
{
    return (uint32_t)(x + y * grid->dims.x + z * grid->dims.x * grid->dims.y);
}


SpatialGrid *SpatialGrid_Create(float cellSize);
void         SpatialGrid_Init(SpatialGrid *grid, float cellSize);
void         SpatialGrid_Deinit(SpatialGrid *grid);
void         SpatialGrid_Destroy(SpatialGrid *grid);
void         SpatialGrid_BuildFromAABBs(SpatialGrid *grid, const SpatialAABB *aabbs, uint32_t count);
void         SpatialGrid_BuildFromTris(SpatialGrid *grid, const SolTri *tris, uint32_t triCount);
void         SpatialGrid_Clear(SpatialGrid *grid);