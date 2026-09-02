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

typedef struct
{
    // 16-byte aligned SIMD members grouped together
    vec4s min;            // {min.x, min.y, min.z, 0.0f}
    vec4s max;            // {max.x, max.y, max.z, 0.0f}
    vec4s invCellSizeVec; // {1/cellSize, 1/cellSize, 1/cellSize, 0.0f}

    float    cellSize, invCellSize;
    ivec3s   dims;        // Grid cell dimensions (x, y, z)
    uint32_t totalCells;  // Total volume (dims.x * dims.y * dims.z)

    void     *memory_block; // <--- The ONLY heap ownership handle
    GridCell *cells;        // View into memory_block
    uint32_t *index_buffer; // View into memory_block
    uint32_t  item_count;
} SpatialGrid;

SpatialGrid *SpatialGrid_Create(float cellSize);
void         SpatialGrid_Init(SpatialGrid *grid, float cellSize);
void         SpatialGrid_Deinit(SpatialGrid *grid);
void         SpatialGrid_Destroy(SpatialGrid *grid);
void         SpatialGrid_BuildFromAABBs(SpatialGrid *grid, const SpatialAABB *aabbs, uint32_t count);
void         SpatialGrid_BuildFromTris(SpatialGrid *grid, const SolTri *tris, uint32_t triCount);
void         SpatialGrid_Clear(SpatialGrid *grid);

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