#include "sol_core.h"
#include <cglm/struct.h>

// Fill the worlds dynamic spatial table
void Sol_System_Spatial_Step(World *world, double dt, double time)
{
    SpatialTable *dynamic = &world->worldSpatial.dynamicUnits;
    SpatialTable_Clear(dynamic);

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & HAS_BODY3))
            continue;

        vec3s pos = world->xforms[id].pos;
        int ix = (int)floorf(pos.x / SPATIAL_CELL_SIZE);
        int iy = (int)floorf(pos.y / SPATIAL_CELL_SIZE);
        int iz = (int)floorf(pos.z / SPATIAL_CELL_SIZE);
        u32 hash = HashCoords(ix, iy, iz);
        SpatialTable_Insert(dynamic, hash, (u32)id);
    }
}

void Sol_Spatial_AddStatic(World *world, SolModel *model)
{
    WorldSpatial *ws = &world->worldSpatial;

    u32 totalTris = 0;
    for (u32 m = 0; m < model->meshCount; m++)
        totalTris += model->meshes[m].indexCount / 3;

    if (ws->tris)
        free(ws->tris);
    ws->tris = malloc(sizeof(CollisionTri) * totalTris);
    ws->triCount = totalTris;

    // Reset static table
    SpatialTable_Clear(&ws->staticWorld);

    u32 triIdx = 0;
    for (u32 m = 0; m < model->meshCount; m++)
    {
        SolMesh *mesh = &model->meshes[m];
        for (u32 i = 0; i < mesh->indexCount; i += 3)
        {
            CollisionTri *t = &ws->tris[triIdx];

            t->a = *(vec3s *)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 0]].position;
            t->b = *(vec3s *)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 1]].position;
            t->c = *(vec3s *)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 2]].position;

            vec3s edge1 = glms_vec3_sub(t->b, t->a);
            vec3s edge2 = glms_vec3_sub(t->c, t->a);
            t->normal = glms_vec3_normalize(glms_vec3_cross(edge1, edge2));

            float minX = fminf(t->a.x, fminf(t->b.x, t->c.x));
            float maxX = fmaxf(t->a.x, fmaxf(t->b.x, t->c.x));
            float minY = fminf(t->a.y, fminf(t->b.y, t->c.y));
            float maxY = fmaxf(t->a.y, fmaxf(t->b.y, t->c.y));
            float minZ = fminf(t->a.z, fminf(t->b.z, t->c.z));
            float maxZ = fmaxf(t->a.z, fmaxf(t->b.z, t->c.z));

            int x0 = (int)floorf(minX / SPATIAL_CELL_SIZE);
            int x1 = (int)floorf(maxX / SPATIAL_CELL_SIZE);
            int y0 = (int)floorf(minY / SPATIAL_CELL_SIZE);
            int y1 = (int)floorf(maxY / SPATIAL_CELL_SIZE);
            int z0 = (int)floorf(minZ / SPATIAL_CELL_SIZE);
            int z1 = (int)floorf(maxZ / SPATIAL_CELL_SIZE);

            for (int x = x0; x <= x1; x++)
                for (int y = y0; y <= y1; y++)
                    for (int z = z0; z <= z1; z++)
                        SpatialTable_Insert(&ws->staticWorld, HashCoords(x, y, z), triIdx);

            triIdx++;
        }
    }
    printf("Static world: %u tris, %u spatial entries\n", totalTris, ws->staticWorld.count);
}

u32 HashCoords(int x, int y, int z)
{
    unsigned int h = ((unsigned int)x * 73856093) ^
                     ((unsigned int)y * 19349663) ^
                     ((unsigned int)z * 83492791);
    return h % SPATIAL_SIZE;
}

void Spatial_Insert(SpatialTable *table, vec3s pos, CompBody *body, u32 value)
{
    if (table->count >= SPATIAL_ENTRIES)
        return; // Table full!

    int ix = (int)floorf(pos.x / SPATIAL_CELL_SIZE);
    int iy = (int)floorf(pos.y / SPATIAL_CELL_SIZE);
    int iz = (int)floorf(pos.z / SPATIAL_CELL_SIZE);
    u32 hash = HashCoords(ix, iy, iz);
    int n = 0;
    for (int ox = -1; ox <= 1; ox++)
        for (int oy = -1; oy <= 1; oy++)
            for (int oz = -1; oz <= 1; oz++)
                body->neighborHashes[n++] = HashCoords(ix + ox, iy + oy, iz + oz);

    // Get a new entry from the pool
    u32 entryIdx = table->count++;
    table->value[entryIdx] = value;

    // Link it: New entry points to the old head, head points to new entry
    table->next[entryIdx] = table->head[hash];
    table->head[hash] = entryIdx;
}

void SpatialTable_Init(SpatialTable *table, u32 capacity)
{
    table->head = malloc(sizeof(u32) * SPATIAL_SIZE);
    table->value = malloc(sizeof(u32) * capacity);
    table->next = malloc(sizeof(u32) * capacity);
    table->capacity = capacity;
    table->count = 0;
    for (u32 i = 0; i < SPATIAL_SIZE; i++)
        table->head[i] = SPATIAL_NULL;
}

void SpatialTable_Clear(SpatialTable *table)
{
    for (u32 i = 0; i < SPATIAL_SIZE; i++)
        table->head[i] = SPATIAL_NULL;
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
    if (table->count >= table->capacity)
        return;

    u32 idx = table->count++;
    table->value[idx] = value;
    table->next[idx] = table->head[hash];
    table->head[hash] = idx;
}