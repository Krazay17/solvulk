/*
 * File: physx_spatial.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Physx spatial grid
 */
#include "physx_i.h"
#include "sol_math.h"
#include "sol_core.h"
#include "world.h"
#include "profiler.h"
#include "xform/s_xform.h"

void Spatial_Add(World *world, int id, CompBody *body)
{
    CompXform  *xform = &world->xforms[id];
    PhysxGroup *group = body->mass == 0 ? &world->spatial->staticGroup : &world->spatial->dynamicGroup;

    group->ents[id].id = id;
    group->entCount++;

    if (body->shape == SHAPE3_MOD)
    {
        Physx_ParseModel(world, id, group);
    }
}

SpatialCell Spatial_Cell_Get(vec3s pos, float cellSize)
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

void SpatialTable_Init(SpatialTable *table, u32 buckets, u32 capacity, float cellSize)
{
    table->head     = malloc(sizeof(u32) * buckets);
    table->value    = malloc(sizeof(u32) * capacity);
    table->next     = malloc(sizeof(u32) * capacity);
    table->size     = buckets;
    table->capacity = capacity;
    table->cellSize = cellSize;
    SpatialTable_Clear(table);
}

void SpatialTable_Clear(SpatialTable *table)
{
    memset(table->head, 0xFF, sizeof(u32) * table->size);
    table->count = 0;
}

void SpatialTable_Free(SpatialTable *table)
{
    free(table->head);
    free(table->value);
    free(table->next);
}

void SpatialTable_Insert(SpatialTable *table, u32 hash, u32 value)
{
    assert(hash < table->size);
    if (table->count >= table->capacity)
        return;

    u32 idx           = table->count++;
    table->value[idx] = value;
    table->next[idx]  = table->head[hash];
    table->head[hash] = idx;
}

void SpatialTable_Compact(SpatialTable *table)
{
    // 1. Count entries per bucket
    u32 *counts = calloc(table->size, sizeof(u32));
    for (u32 i = 0; i < table->size; i++)
    {
        u32 entry = table->head[i];
        while (entry != SPATIAL_NULL)
        {
            counts[i]++;
            entry = table->next[entry];
        }
    }

    // 2. Compute offsets (prefix sum)
    u32 *offsets = malloc(table->size * sizeof(u32));
    offsets[0]   = 0;
    for (u32 i = 1; i < table->size; i++)
        offsets[i] = offsets[i - 1] + counts[i - 1];

    // 3. Copy values into sorted order
    u32 *sorted = malloc(table->count * sizeof(u32));
    u32 *cursor = malloc(table->size * sizeof(u32));
    memcpy(cursor, offsets, table->size * sizeof(u32));

    for (u32 i = 0; i < table->size; i++)
    {
        u32 entry = table->head[i];
        while (entry != SPATIAL_NULL)
        {
            sorted[cursor[i]++] = table->value[entry];
            entry               = table->next[entry];
        }
    }

    // 4. Rebuild as contiguous — head points to start, next is just +1
    memcpy(table->value, sorted, table->count * sizeof(u32));

    for (u32 i = 0; i < table->size; i++)
    {
        if (counts[i] == 0)
        {
            table->head[i] = SPATIAL_NULL;
        }
        else
        {
            table->head[i] = offsets[i];
            for (u32 j = 0; j < counts[i] - 1; j++)
                table->next[offsets[i] + j] = offsets[i] + j + 1;
            table->next[offsets[i] + counts[i] - 1] = SPATIAL_NULL;
        }
    }

    free(sorted);
    free(counts);
    free(offsets);
    free(cursor);
}

int Spatial_QueryEntsAABB(SpatialTable *table, vec3s min, vec3s max, int *candidates, int maxCount)
{
    if (!candidates || maxCount <= 0)
        return 0;

    float cellSize = table->cellSize;

    // Convert AABB to integer cell coordinate ranges
    int cellMinX = (int)floorf(min.x / cellSize);
    int cellMinY = (int)floorf(min.y / cellSize);
    int cellMinZ = (int)floorf(min.z / cellSize);
    int cellMaxX = (int)floorf(max.x / cellSize);
    int cellMaxY = (int)floorf(max.y / cellSize);
    int cellMaxZ = (int)floorf(max.z / cellSize);

    // Track which entities we've already added (cheap dedup)
    // Using a bitset sized to MAX_ENTS would be cleaner; for now use linear scan
    int count = 0;

    for (int z = cellMinZ; z <= cellMaxZ; z++)
    {
        for (int y = cellMinY; y <= cellMaxY; y++)
        {
            for (int x = cellMinX; x <= cellMaxX; x++)
            {
                u32 hash  = hash_coords(x, y, z) & (table->size - 1);
                u32 entry = table->head[hash];

                while (entry != SPATIAL_NULL)
                {
                    u32 id = table->value[entry];
                    entry  = table->next[entry];

                    // Dedup: an entity in multiple cells, OR a hash collision with an unrelated entity
                    bool seen = false;
                    for (int i = 0; i < count; i++)
                    {
                        if ((u32)candidates[i] == id)
                        {
                            seen = true;
                            break;
                        }
                    }
                    if (seen)
                        continue;

                    if (count >= maxCount)
                        return count;
                    candidates[count++] = id;
                }
            }
        }
    }

    return count;
}