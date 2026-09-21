/*
 * File: spatial_grid.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-12
 *
 */
#pragma once
#include "sol/types.h"
#include "sol_math.h"
#include "sol_buffer.h"

#define QUERY_HASH_SIZE 256 // Power of 2 (1KB on stack)
#define QUERY_HASH_MASK (QUERY_HASH_SIZE - 1)

#define SPATIAL_PACK_ID(id, idx) ((((id) & 0xFFFF) << 16) | ((idx) & 0xFFFF))
#define SPATIAL_UNPACK_IDX(id) ((id) & 0xFFFF)
#define SPATIAL_UNPACK_ID(id) (((id) >> 16) & 0xFFFF)

typedef struct
{
    ivec3s step;
    ivec3s cell;
    vec3s tMax;
    vec3s tDelta;
} GridDDA;

typedef struct
{
    uint32_t id;
    uint32_t stamp;
} HashSlot;

typedef struct SpatialGrid
{
    vec3s min, max;
    float cell_size;
    float cell_size_inv;
    ivec3s dims;
    uint32_t cell_count;

    uint32_t *cell_offsets;
    uint32_t *ids;
    uint32_t *cursor;
} SpatialGrid;

static inline ivec3s SpatialGrid_WorldToCell(const SpatialGrid *grid, vec3s pos)
{
    vec3s final_pos = glms_vec3_scale(glms_vec3_sub(pos, grid->min), grid->cell_size_inv);
    final_pos       = glms_vec3_floor(final_pos);
    return (ivec3s){(int)final_pos.x, (int)final_pos.y, (int)final_pos.z};
}

static inline uint32_t SpatialGrid_GetCellIdx(const SpatialGrid *grid, int x, int y, int z)
{
    x = x < 0 ? 0 : (x >= grid->dims.x ? grid->dims.x - 1 : x);
    y = y < 0 ? 0 : (y >= grid->dims.y ? grid->dims.y - 1 : y);
    z = z < 0 ? 0 : (z >= grid->dims.z ? grid->dims.z - 1 : z);

    return (uint32_t)(x + y * grid->dims.x + z * grid->dims.x * grid->dims.y);
}

static inline uint32_t SpatialGrid_Resize(SpatialGrid *grid, vec3s min, vec3s max, float cell_size)
{
    if (cell_size <= 0.0f)
        return 0;

    grid->min           = min;
    grid->max           = max;
    grid->cell_size     = cell_size;
    grid->cell_size_inv = 1.0f / cell_size;

    grid->dims.x = (uint32_t)ceilf((max.x - min.x) * grid->cell_size_inv);
    grid->dims.y = (uint32_t)ceilf((max.y - min.y) * grid->cell_size_inv);
    grid->dims.z = (uint32_t)ceilf((max.z - min.z) * grid->cell_size_inv);

    grid->cell_count = grid->dims.x * grid->dims.y * grid->dims.z;

    // solb_reserve is a no-op if existing capacity already covers this size —
    // only actually reallocs when cell_count grew past current capacity.
    solb_reserve(grid->cell_offsets, grid->cell_count + 1);
    solb_set_count(grid->cell_offsets, grid->cell_count + 1);

    solb_reserve(grid->cursor, grid->cell_count);
    solb_set_count(grid->cursor, grid->cell_count);

    return grid->cell_count;
}

static inline uint32_t SpatialGrid_Init(SpatialGrid *grid, vec3s min, vec3s max, float cell_size)
{
    grid->cell_offsets = NULL; // only safe to null here, once, before any allocation exists
    grid->cursor       = NULL;
    solb_init(grid->ids, 64);

    return SpatialGrid_Resize(grid, min, max, cell_size);
}

static inline void SpatialGrid_Deinit(SpatialGrid *grid)
{
    solb_free(grid->ids);
    solb_free(grid->cell_offsets);
    solb_free(grid->cursor);
    free(grid);
}

static inline void SpatialGrid_FitBoundsToItems(SpatialGrid *grid, vec3s *mins, vec3s *maxs,
                                                uint32_t item_count)
{
    if (item_count == 0)
        return;

    vec3s min = mins[0];
    vec3s max = maxs[0];

    for (uint32_t i = 1; i < item_count; i++)
    {
        min           = glms_vec3_minv(min, mins[i]);
        max           = glms_vec3_maxv(max, maxs[i]);
    }

    // small margin so items exactly on the boundary don't clamp oddly
    min = glms_vec3_subs(min, grid->cell_size);
    max = glms_vec3_adds(max, grid->cell_size);

    SpatialGrid_Resize(grid, min, max, grid->cell_size); // recompute dims/cell_count from tight bounds
}

static inline uint32_t SpatialGrid_Build(SpatialGrid *grid, uint32_t *ids, vec3s *mins, vec3s *maxs,
                                         uint32_t item_count)
{
    SpatialGrid_FitBoundsToItems(grid, mins, maxs, item_count);
    // --- Pass 1: count how many spans land in each cell ---
    // offsets[0] stays 0; counts go into offsets[i+1] temporarily.
    memset(grid->cell_offsets, 0, sizeof(uint32_t) * (grid->cell_count + 1));

    for (uint32_t i = 0; i < item_count; i++)
    {
        ivec3s min_cell = SpatialGrid_WorldToCell(grid, mins[i]);
        ivec3s max_cell = SpatialGrid_WorldToCell(grid, maxs[i]);

        for (int z = min_cell.z; z <= max_cell.z; z++)
            for (int y = min_cell.y; y <= max_cell.y; y++)
                for (int x = min_cell.x; x <= max_cell.x; x++)
                {
                    uint32_t cell_idx = SpatialGrid_GetCellIdx(grid, x, y, z);
                    grid->cell_offsets[cell_idx + 1]++;
                }
    }

    // --- Pass 2: prefix sum turns counts into real offsets ---
    for (uint32_t i = 0; i < grid->cell_count; i++)
        grid->cell_offsets[i + 1] += grid->cell_offsets[i];

    uint32_t total = grid->cell_offsets[grid->cell_count];
    solb_reserve(grid->ids, total);
    solb_set_count(grid->ids, total);

    memcpy(grid->cursor, grid->cell_offsets, sizeof(uint32_t) * grid->cell_count);

    for (uint32_t i = 0; i < item_count; i++)
    {
        uint32_t id       = ids[i];
        ivec3s min_cell = SpatialGrid_WorldToCell(grid, mins[i]);
        ivec3s max_cell = SpatialGrid_WorldToCell(grid, maxs[i]);

        for (int z = min_cell.z; z <= max_cell.z; z++)
            for (int y = min_cell.y; y <= max_cell.y; y++)
                for (int x = min_cell.x; x <= max_cell.x; x++)
                {
                    uint32_t cell_idx = SpatialGrid_GetCellIdx(grid, x, y, z);

                    grid->ids[grid->cursor[cell_idx]++] = id;
                }
    }

    return total;
}

// Returns true if `id` is new this generation (and records it as seen).
// Returns false if `id` was already recorded — caller should skip it.
static inline bool SpatialGrid_HashInsert(HashSlot *table, uint32_t gen, uint32_t id)
{
    uint32_t slot = (id * 2654435761u) & QUERY_HASH_MASK;
    for (uint32_t probe = 0; probe < QUERY_HASH_SIZE; probe++)
    {
        if (table[slot].stamp != gen)
        {
            table[slot].id    = id;
            table[slot].stamp = gen;
            return true;
        }
        if (table[slot].id == id)
            return false;
        slot = (slot + 1) & QUERY_HASH_MASK;
    }
    return false; // table full — degrade safely by treating as "seen" (drops dedup, doesn't crash)
}

static inline uint32_t SpatialGrid_Query(SpatialGrid *grid, vec3s min, vec3s max, IdBuffer *id_buf)
{
    ivec3s min_cell = SpatialGrid_WorldToCell(grid, min);
    ivec3s max_cell = SpatialGrid_WorldToCell(grid, max);

    static _Thread_local HashSlot hash_table[QUERY_HASH_SIZE];
    static _Thread_local uint32_t local_gen = 0;
    uint32_t this_gen                       = ++local_gen;

    solb_set_count(id_buf->ids, 0); // reset for THIS query — not once per phase

    for (int z = min_cell.z; z <= max_cell.z; z++)
        for (int y = min_cell.y; y <= max_cell.y; y++)
            for (int x = min_cell.x; x <= max_cell.x; x++)
            {
                uint32_t cell_idx = SpatialGrid_GetCellIdx(grid, x, y, z);
                uint32_t start    = grid->cell_offsets[cell_idx];
                uint32_t end      = grid->cell_offsets[cell_idx + 1];

                for (uint32_t i = start; i < end; i++)
                {
                    uint32_t id = grid->ids[i];
                    if (SpatialGrid_HashInsert(hash_table, this_gen, id))
                        solb_push(id_buf->ids, id);
                }
            }

    return solb_count(id_buf->ids);
}

// #########################
// ####### RAY TRACE #######
// #########################

// dir MUST already be normalized — tDelta below assumes unit length.
static inline GridDDA GridDDA_Init(const SpatialGrid *grid, vec3s start, vec3s dir)
{
    GridDDA it;
    it.cell = SpatialGrid_WorldToCell(grid, start);

    it.step.x = (dir.x > 0.0f) ? 1 : (dir.x < 0.0f ? -1 : 0);
    it.step.y = (dir.y > 0.0f) ? 1 : (dir.y < 0.0f ? -1 : 0);
    it.step.z = (dir.z > 0.0f) ? 1 : (dir.z < 0.0f ? -1 : 0);

    float cs = grid->cell_size;

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

// Advances to the next cell boundary crossed; returns the ray distance at which it happened.
static inline float GridDDA_Step(GridDDA *it)
{
    float t;
    if (it->tMax.x < it->tMax.y)
    {
        if (it->tMax.x < it->tMax.z)
        {
            t = it->tMax.x;
            it->cell.x += it->step.x;
            it->tMax.x += it->tDelta.x;
        }
        else
        {
            t = it->tMax.z;
            it->cell.z += it->step.z;
            it->tMax.z += it->tDelta.z;
        }
    }
    else
    {
        if (it->tMax.y < it->tMax.z)
        {
            t = it->tMax.y;
            it->cell.y += it->step.y;
            it->tMax.y += it->tDelta.y;
        }
        else
        {
            t = it->tMax.z;
            it->cell.z += it->step.z;
            it->tMax.z += it->tDelta.z;
        }
    }
    return t;
}