#include "sol_core.h"
#include <cglm/struct.h>

static u32 HashCoords(int x, int y, int z)
{
    unsigned int h = ((unsigned int)x * 73856093) ^
                     ((unsigned int)y * 19349663) ^
                     ((unsigned int)z * 83492791);
    return h % SPATIAL_SIZE;
}

static void Spatial_Insert(SpatialTable *table, vec3s pos, u32 value)
{
    if (table->count >= SPATIAL_ENTRIES)
        return; // Table full!

    int ix = (int)floorf(pos.x / CELL_SIZE);
    int iy = (int)floorf(pos.y / CELL_SIZE);
    int iz = (int)floorf(pos.z / CELL_SIZE);
    u32 hash = HashCoords(ix, iy, iz);

    // Get a new entry from the pool
    u32 entryIdx = table->count++;
    table->value[entryIdx] = value;

    // Link it: New entry points to the old head, head points to new entry
    table->next[entryIdx] = table->head[hash];
    table->head[hash] = entryIdx;
}

void Sol_System_Spatial_Step(World *world, double dt, double time)
{
    SpatialTable *dynamic = &world->worldSpatial.dynamicUnits;

    // 1. Reset the table (Very fast)
    for (int i = 0; i < SPATIAL_SIZE; i++)
        dynamic->head[i] = SOL_SPATIAL_NULL;
    dynamic->count = 0;

    // 2. Insert all entities with physical bodies
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & HAS_BODY3))
            continue;

        // Use the insertion helper we wrote earlier
        Spatial_Insert(dynamic, world->xforms[id].pos, (u32)id);
    }
}

void Sol_Spatial_AddStatic(World *world, SolModel *model)
{
    // SpatialTable *table = &world->worldSpatial.staticWorld;

    // // Initialize heads to NULL
    // for(int i = 0; i < SPATIAL_SIZE; i++) table->head[i] = SOL_SPATIAL_NULL;
    // table->count = 0;

    // u32 triGlobalIdx = 0;
    // for (u32 m = 0; m < model->meshCount; m++) {
    //     SolMesh *mesh = &model->meshes[m];
    //     for (u32 i = 0; i < mesh->indexCount; i += 3) {
    //         // Get triangle vertices
    //         vec3s a = *(vec3s*)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 0]].position;
    //         vec3s b = *(vec3s*)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 1]].position;
    //         vec3s c = *(vec3s*)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 2]].position;

    //         // 1. Calculate AABB of triangle
    //         float minX = fminf(a.x, fminf(b.x, c.x));
    //         float maxX = fmaxf(a.x, fmaxf(b.x, c.x));
    //         // ... repeat for Y and Z ...

    //         // 2. Find cell range
    //         int x0 = (int)floorf(minX / CELL_SIZE);
    //         int x1 = (int)floorf(maxX / CELL_SIZE);
    //         // ... repeat for Y and Z ...

    //         // 3. Insert triangle index into every cell it overlaps
    //         for (int x = x0; x <= x1; x++) {
    //             for (int y = y0; y <= y1; y++) {
    //                 for (int z = z0; z <= z1; z++) {
    //                     if (table->count >= SPATIAL_ENTRIES) break;

    //                     u32 hash = HashCoords(x, y, z);
    //                     u32 entryIdx = table->count++;
    //                     table->value[entryIdx] = triGlobalIdx; // Store triangle ID
    //                     table->next[entryIdx] = table->head[hash];
    //                     table->head[hash] = entryIdx;
    //                 }
    //             }
    //         }
    //         triGlobalIdx++;
    //     }
    // }
}

WorldCollider worldCollider = {0};

void Sol_Physics_BuildWorldCollider(World *world, SolModel *model)
{
    // WorldCollider *WorldCollider = &world->worldSpatial.staticWorld;

    // // Count triangles
    // uint32_t totalTris = 0;
    // for (uint32_t m = 0; m < model->meshCount; m++)
    //     totalTris += model->meshes[m].indexCount / 3;

    // worldCollider.tris = malloc(sizeof(CollisionTri) * totalTris);
    // worldCollider.triCount = totalTris;
    // memset(worldCollider.cells, 0, sizeof(worldCollider.cells));

    // // Extract triangles
    // uint32_t triIdx = 0;
    // for (uint32_t m = 0; m < model->meshCount; m++)
    // {
    //     SolMesh *mesh = &model->meshes[m];
    //     for (uint32_t i = 0; i < mesh->indexCount; i += 3)
    //     {
    //         SolVertex *va = &model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 0]];
    //         SolVertex *vb = &model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 1]];
    //         SolVertex *vc = &model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 2]];

    //         CollisionTri *tri = &worldCollider.tris[triIdx];
    //         tri->a = (vec3s){va->position[0], va->position[1], va->position[2]};
    //         tri->b = (vec3s){vb->position[0], vb->position[1], vb->position[2]};
    //         tri->c = (vec3s){vc->position[0], vc->position[1], vc->position[2]};

    //         // Face normal
    //         vec3s edge1 = glms_vec3_sub(tri->b, tri->a);
    //         vec3s edge2 = glms_vec3_sub(tri->c, tri->a);
    //         tri->normal = glms_vec3_normalize(glms_vec3_cross(edge1, edge2));

    //         // Insert into every cell the triangle's AABB touches
    //         float minX = fminf(fminf(tri->a.x, tri->b.x), tri->c.x);
    //         float minY = fminf(fminf(tri->a.y, tri->b.y), tri->c.y);
    //         float minZ = fminf(fminf(tri->a.z, tri->b.z), tri->c.z);
    //         float maxX = fmaxf(fmaxf(tri->a.x, tri->b.x), tri->c.x);
    //         float maxY = fmaxf(fmaxf(tri->a.y, tri->b.y), tri->c.y);
    //         float maxZ = fmaxf(fmaxf(tri->a.z, tri->b.z), tri->c.z);

    //         int x0 = (int)floorf(minX / WORLD_CELL_SIZE);
    //         int y0 = (int)floorf(minY / WORLD_CELL_SIZE);
    //         int z0 = (int)floorf(minZ / WORLD_CELL_SIZE);
    //         int x1 = (int)floorf(maxX / WORLD_CELL_SIZE);
    //         int y1 = (int)floorf(maxY / WORLD_CELL_SIZE);
    //         int z1 = (int)floorf(maxZ / WORLD_CELL_SIZE);

    //         for (int x = x0; x <= x1; x++)
    //             for (int y = y0; y <= y1; y++)
    //                 for (int z = z0; z <= z1; z++)
    //                 {
    //                     WorldCell *cell = &worldCollider.cells[WorldHash(x, y, z)];
    //                     if (cell->count < MAX_TRIS_PER_CELL)
    //                         cell->triIndices[cell->count++] = triIdx;
    //                 }

    //         triIdx++;
    //     }
    // }
    // printf("WorldCollider: %u triangles\n", totalTris);
}
