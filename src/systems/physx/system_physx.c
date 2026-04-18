#include <cglm/struct.h>
#include <omp.h>

#include "sol_core.h"

static ResolveShapeTri shapeTriResolvers[BODY_SHAPE_COUNT] = {
    [BODY_SHAPE_SPHERE] = ResolveSphereTriangle,
    [BODY_SHAPE_CAPSULE] = ResolveCapsuleTriangle,
};

static int ents[MAX_ENTS];

static SolProfiler profSpatialStatic = {.name = "Spatial Static"};
static SolProfiler profSpatialDynamic = {.name = "Spatial Dynamic"};
static int profFrame = 0;

void Sol_System_Step_Physx_3d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY3 | HAS_XFORM;
    int i, j, k;
    int count = 0;
    int actives = world->activeCount;
    WorldSpatial *ws = &world->worldSpatial;

    for (k = 0; k < actives; k++)
    {
        int id = world->activeEntities[k];
        if ((world->masks[id] & required) != required)
            continue;
        ents[count++] = id;
    }

    Prof_Begin(&profSpatialStatic);
    // Static position resolution
#pragma omp parallel for if (count > 1000) schedule(dynamic, 16)
    for (i = 0; i < count; i++)
    {
        int id = ents[i];
        CompXform *xform = &world->xforms[id];
        CompBody *body = &world->bodies[id];

        body->grounded = 0;
        if (xform->pos.y < -15.0f)
        {
            xform->pos = (vec3s){0, 100, 0};
            continue;
        }

        vec3s accel = SOL_PHYS_GRAV;
        accel = glms_vec3_add(accel, body->force);
        accel = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));

        static_collisions_grid(body, xform, substep_get(body, fdt), &world->staticGrid, ws);
    }
    Prof_End(&profSpatialStatic);

    Sol_Spatial_Build_Dynamic(world);

    Prof_Begin(&profSpatialDynamic);
    // 2. Dynamic position resolution
#pragma omp parallel for if (count > 1000) schedule(dynamic, 16)
    for (j = 0; j < count; j++)
    {
        int id = ents[j];
        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        if (body->invMass <= 0.0f)
            continue;

        dynamic_collisions_hashed(world, id, body, xform);
    }
    Prof_End(&profSpatialDynamic);

    profFrame++;
    if (profFrame % 200 == 0)
    {
        Prof_Print(&profSpatialStatic);
        Prof_Print(&profSpatialDynamic);
    }
}

// Per-entity substep count based on speed
SubstepData substep_get(CompBody *body, float fdt)
{
    SubstepData substep_data = {0};

    float speed = glms_vec3_norm(body->vel);
    float stepDist = body->radius * 0.9f;
    u8 substeps = (int)ceilf(speed * fdt / stepDist);
    if (substeps < 1)
        substeps = 1;
    if (substeps > 8)
        substeps = 8;

    substep_data.substeps = substeps;
    substep_data.sub_dt = fdt / substeps;

    return substep_data;
}

void dynamic_collisions_hashed(World *world, int id, CompBody *body, CompXform *xform)
{
    WorldSpatial *ws = &world->worldSpatial;
    SpatialCell cell = GetSpatialCell(xform->pos);
    for (int n = 0; n < 27; n++)
    {
        u32 entry = ws->dynamicUnits.head[cell.neighborHashes[n] & ws->dynamicUnits.mask];
        while (entry != SPATIAL_NULL)
        {
            u32 otherID = ws->dynamicUnits.value[entry];
            if (id < otherID)
                ResolveCollision(body, xform,
                                 &world->bodies[otherID],
                                 &world->xforms[otherID]);
            entry = ws->dynamicUnits.next[entry];
        }
    }
}

void static_collisions_hashed(CompBody *body, CompXform *xform, SubstepData substep_data, WorldSpatial *ws)
{
    int totalChecks = 0;
    for (int s = 0; s < substep_data.substeps; s++)
    {
        xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, substep_data.sub_dt));
        SpatialCell cell = GetSpatialCell(xform->pos);
        for (int n = 0; n < 27; n++)
        {
            u32 entry = ws->staticWorld.head[cell.neighborHashes[n] & ws->staticWorld.mask];
            while (entry != SPATIAL_NULL)
            {
                if (++totalChecks > 512)
                    return;
                CollisionTri *tri = &ws->tris[ws->staticWorld.value[entry]];
                SolCollision col = ResolveSphereTriangle(body, xform, tri);
                if (col.didCollide && col.normal.y > 0.5f)
                    body->grounded = 1;
                entry = ws->staticWorld.next[entry];
            }
        }
    }
}

void static_collisions_grid(CompBody *body, CompXform *xform, SubstepData substep_data, StaticGrid *grid, WorldSpatial *ws)
{
    ResolveShapeTri resolve = shapeTriResolvers[body->shape];

    for (int s = 0; s < substep_data.substeps; s++)
    {
        xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, substep_data.sub_dt));

        int checks = 0;
        int ix = (int)floorf((xform->pos.x - grid->worldMin.x) / grid->cellSize);
        int iy = (int)floorf((xform->pos.y - grid->worldMin.y) / grid->cellSize);
        int iz = (int)floorf((xform->pos.z - grid->worldMin.z) / grid->cellSize);

        for (int ox = -1; ox <= 1; ox++)
            for (int oy = -1; oy <= 1; oy++)
                for (int oz = -1; oz <= 1; oz++)
                {
                    int cx = ix + ox, cy = iy + oy, cz = iz + oz;
                    if (cx < 0 || cx >= grid->gridX ||
                        cy < 0 || cy >= grid->gridY ||
                        cz < 0 || cz >= grid->gridZ)
                        continue;

                    u32 cell = cx + cy * grid->gridX + cz * grid->gridX * grid->gridY;
                    u32 start = grid->offsets[cell];
                    u32 end = grid->offsets[cell + 1];

                    for (u32 e = start; e < end; e++)
                    {
                        if (++checks > 1000)
                            return;

                        CollisionTri *tri = &ws->tris[grid->tris[e]];
                        SolCollision col = resolve(body, xform, tri);

                        if (col.didCollide && col.normal.y > 0.5f)
                            body->grounded = 1;
                    }
                }
    }
}

void Sol_System_Step_Physx_2d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY2 | HAS_XFORM;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompBody *body = &world->bodies[id];
            CompMovement *movement = &world->movements[id];
            CompXform *xform = &world->xforms[id];

            float moveGrav = MOVE_STATE_FORCES[movement->configId][movement->moveState].gravity;
            body->vel.y = moveGrav ? body->vel.y + moveGrav * fdt
                                   : body->vel.y + SOL_PHYS_GRAV.y * fdt;
            vec3s finalVel = glms_vec3_scale(body->vel, fdt * 20.0f);
            xform->pos = glms_vec3_add(xform->pos, finalVel);

            if (xform->pos.y + body->height >= Sol_GetState()->windowHeight)
            {
                xform->pos.y = Sol_GetState()->windowHeight - body->height;
                body->vel.y = 0;
            }
        }
    }
}

void resolve_position_only(CompBody *aBody, CompXform *aXform,
                           CompBody *bBody, CompXform *bXform)
{
    vec3s delta = glms_vec3_sub(aXform->pos, bXform->pos);
    float combined = aBody->radius + bBody->radius;
    float distSq = glms_vec3_dot(delta, delta);

    if (distSq >= combined * combined || distSq <= 0.0f)
        return;

    float dist = sqrtf(distSq);
    vec3s normal = glms_vec3_scale(delta, 1.0f / dist);
    float penetration = combined - dist;
    float totalInvMass = aBody->invMass + bBody->invMass;
    if (totalInvMass <= 0.0f)
        return;

    float correction = fmaxf(penetration - SOL_PHYS_COLLISION_SKIN, 0.0f) / totalInvMass;
    vec3s corrVec = glms_vec3_scale(normal, correction);

    aXform->pos = glms_vec3_add(aXform->pos, glms_vec3_scale(corrVec, aBody->invMass));
    bXform->pos = glms_vec3_sub(bXform->pos, glms_vec3_scale(corrVec, bBody->invMass));
}

void resolve_velocity_only(CompBody *aBody, CompXform *aXform,
                           CompBody *bBody, CompXform *bXform)
{
    vec3s delta = glms_vec3_sub(aXform->pos, bXform->pos);
    float combined = aBody->radius + bBody->radius;
    float distSq = glms_vec3_dot(delta, delta);

    // Small margin so we catch pairs that are touching after position solve
    float margin = combined + SOL_PHYS_COLLISION_SKIN;
    if (distSq >= margin * margin || distSq <= 0.0f)
        return;

    float dist = sqrtf(distSq);
    vec3s normal = glms_vec3_scale(delta, 1.0f / dist);

    vec3s relVel = glms_vec3_sub(aBody->vel, bBody->vel);
    float velAlongNormal = glms_vec3_dot(relVel, normal);
    if (velAlongNormal > 0)
        return;

    float totalInvMass = aBody->invMass + bBody->invMass;
    if (totalInvMass <= 0.0f)
        return;

    float e = fminf(aBody->restitution, bBody->restitution);
    float j = -(1.0f + e) * velAlongNormal / totalInvMass;
    vec3s impulse = glms_vec3_scale(normal, j);

    aBody->vel = glms_vec3_add(aBody->vel, glms_vec3_scale(impulse, aBody->invMass));
    bBody->vel = glms_vec3_sub(bBody->vel, glms_vec3_scale(impulse, bBody->invMass));
}

// Fill the worlds dynamic spatial table
void Sol_Spatial_Build_Dynamic(World *world)
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
        u32 hash = HashCoords(ix, iy, iz, dynamic->mask);
        SpatialTable_Insert(dynamic, hash, (u32)id);
    }
}

void Sol_Spatial_AddStatic(World *world, SolModel *model, CompXform *xform)
{
    WorldSpatial *ws = &world->worldSpatial;

    u32 totalTris = 0;
    for (u32 m = 0; m < model->meshCount; m++)
        totalTris += model->meshes[m].indexCount / 3;

    u32 oldCount = ws->triCount;
    totalTris += oldCount;

    ws->tris = realloc(ws->tris, sizeof(CollisionTri) * totalTris);
    ws->triCount = totalTris;

    mat3s mat = glms_quat_mat3(xform->quat);

    u32 triIdx = oldCount;
    for (u32 m = 0; m < model->meshCount; m++)
    {
        SolMesh *mesh = &model->meshes[m];
        for (u32 i = 0; i < mesh->indexCount; i += 3)
        {
            CollisionTri *t = &ws->tris[triIdx];

            vec3s verts[3] = {
                *(vec3s *)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 0]].position,
                *(vec3s *)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 1]].position,
                *(vec3s *)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 2]].position,
            };

            for (int v = 0; v < 3; v++)
            {
                verts[v] = glms_vec3_mul(verts[v], xform->scale);
                verts[v] = glms_mat3_mulv(mat, verts[v]);
                verts[v] = glms_vec3_add(verts[v], xform->pos);
            }

            t->a = verts[0];
            t->b = verts[1];
            t->c = verts[2];

            t->center = glms_vec3_scale(
                glms_vec3_add(glms_vec3_add(t->a, t->b), t->c), 1.0f / 3.0f);
            float da = glms_vec3_norm(glms_vec3_sub(t->a, t->center));
            float db = glms_vec3_norm(glms_vec3_sub(t->b, t->center));
            float dc = glms_vec3_norm(glms_vec3_sub(t->c, t->center));
            t->boundRadius = fmaxf(da, fmaxf(db, dc));

            vec3s edge1 = glms_vec3_sub(t->b, t->a);
            vec3s edge2 = glms_vec3_sub(t->c, t->a);
            vec3s cross = glms_vec3_cross(edge1, edge2);
            float len = glms_vec3_norm(cross);
            if (len > 0.00001f)
                t->normal = glms_vec3_scale(cross, 1.0f / len);
            else
                t->normal = (vec3s){0, 1, 0}; // Default up normal for broken tris

            triIdx++;
        }
    }
    printf("Static world: %u tris, %u spatial entries\n", totalTris, ws->staticWorld.count);
}

void spatial_hash_tris_static(WorldSpatial *ws)
{
    CollisionTri *t = ws->tris;
    int count = ws->triCount;
    for (int i = 0; i < count; i++)
    {
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
                    SpatialTable_Insert(&ws->staticWorld, HashCoords(x, y, z, ws->staticWorld.mask), i);
    }
}

void StaticGrid_Build(StaticGrid *grid, WorldSpatial *ws,
                      vec3s worldMin, vec3s worldMax, float cellSize)
{
    grid->cellSize = cellSize;
    grid->worldMin = worldMin;
    grid->gridX = (int)ceilf((worldMax.x - worldMin.x) / cellSize) + 1;
    grid->gridY = (int)ceilf((worldMax.y - worldMin.y) / cellSize) + 1;
    grid->gridZ = (int)ceilf((worldMax.z - worldMin.z) / cellSize) + 1;
    u32 cellCount = grid->gridX * grid->gridY * grid->gridZ;

    // 1. Count entries per cell
    u32 *counts = calloc(cellCount, sizeof(u32));
    u32 totalEntries = 0;

    for (u32 t = 0; t < ws->triCount; t++)
    {
        CollisionTri *tri = &ws->tris[t];
        float minX = fminf(tri->a.x, fminf(tri->b.x, tri->c.x));
        float maxX = fmaxf(tri->a.x, fmaxf(tri->b.x, tri->c.x));
        float minY = fminf(tri->a.y, fminf(tri->b.y, tri->c.y));
        float maxY = fmaxf(tri->a.y, fmaxf(tri->b.y, tri->c.y));
        float minZ = fminf(tri->a.z, fminf(tri->b.z, tri->c.z));
        float maxZ = fmaxf(tri->a.z, fmaxf(tri->b.z, tri->c.z));

        int x0 = (int)floorf((minX - worldMin.x) / cellSize);
        int x1 = (int)floorf((maxX - worldMin.x) / cellSize);
        int y0 = (int)floorf((minY - worldMin.y) / cellSize);
        int y1 = (int)floorf((maxY - worldMin.y) / cellSize);
        int z0 = (int)floorf((minZ - worldMin.z) / cellSize);
        int z1 = (int)floorf((maxZ - worldMin.z) / cellSize);

        x0 = x0 < 0 ? 0 : x0;
        x1 = x1 >= grid->gridX ? grid->gridX - 1 : x1;
        y0 = y0 < 0 ? 0 : y0;
        y1 = y1 >= grid->gridY ? grid->gridY - 1 : y1;
        z0 = z0 < 0 ? 0 : z0;
        z1 = z1 >= grid->gridZ ? grid->gridZ - 1 : z1;

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++)
                {
                    counts[x + y * grid->gridX + z * grid->gridX * grid->gridY]++;
                    totalEntries++;
                }
    }

    // 2. Prefix sum → offsets
    grid->offsets = malloc((cellCount + 1) * sizeof(u32));
    grid->offsets[0] = 0;
    for (u32 i = 0; i < cellCount; i++)
        grid->offsets[i + 1] = grid->offsets[i] + counts[i];

    // 3. Fill sorted triangle indices
    grid->tris = malloc(totalEntries * sizeof(u32));
    u32 *cursor = malloc(cellCount * sizeof(u32));
    memcpy(cursor, grid->offsets, cellCount * sizeof(u32));

    for (u32 t = 0; t < ws->triCount; t++)
    {
        CollisionTri *tri = &ws->tris[t];
        float minX = fminf(tri->a.x, fminf(tri->b.x, tri->c.x));
        float maxX = fmaxf(tri->a.x, fmaxf(tri->b.x, tri->c.x));
        float minY = fminf(tri->a.y, fminf(tri->b.y, tri->c.y));
        float maxY = fmaxf(tri->a.y, fmaxf(tri->b.y, tri->c.y));
        float minZ = fminf(tri->a.z, fminf(tri->b.z, tri->c.z));
        float maxZ = fmaxf(tri->a.z, fmaxf(tri->b.z, tri->c.z));

        int x0 = (int)floorf((minX - worldMin.x) / cellSize);
        int x1 = (int)floorf((maxX - worldMin.x) / cellSize);
        int y0 = (int)floorf((minY - worldMin.y) / cellSize);
        int y1 = (int)floorf((maxY - worldMin.y) / cellSize);
        int z0 = (int)floorf((minZ - worldMin.z) / cellSize);
        int z1 = (int)floorf((maxZ - worldMin.z) / cellSize);

        x0 = x0 < 0 ? 0 : x0;
        x1 = x1 >= grid->gridX ? grid->gridX - 1 : x1;
        y0 = y0 < 0 ? 0 : y0;
        y1 = y1 >= grid->gridY ? grid->gridY - 1 : y1;
        z0 = z0 < 0 ? 0 : z0;
        z1 = z1 >= grid->gridZ ? grid->gridZ - 1 : z1;

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++)
                {
                    u32 cell = x + y * grid->gridX + z * grid->gridX * grid->gridY;
                    grid->tris[cursor[cell]++] = t;
                }
    }

    free(counts);
    free(cursor);

    // Find worst cell for debug
    u32 worst = 0;
    for (u32 i = 0; i < cellCount; i++)
    {
        u32 len = grid->offsets[i + 1] - grid->offsets[i];
        if (len > worst)
            worst = len;
    }
    printf("StaticGrid: %dx%dx%d cells, %u entries, worst cell: %u\n",
           grid->gridX, grid->gridY, grid->gridZ, totalEntries, worst);
}

SpatialCell GetSpatialCell(vec3s pos)
{
    SpatialCell cell;
    cell.ix = (int)floorf(pos.x / SPATIAL_CELL_SIZE);
    cell.iy = (int)floorf(pos.y / SPATIAL_CELL_SIZE);
    cell.iz = (int)floorf(pos.z / SPATIAL_CELL_SIZE);

    int n = 0;
    for (int ox = -1; ox <= 1; ox++)
        for (int oy = -1; oy <= 1; oy++)
            for (int oz = -1; oz <= 1; oz++)
                cell.neighborHashes[n++] = HashCoordsRaw(
                    cell.ix + ox, cell.iy + oy, cell.iz + oz);
    return cell;
}

u32 HashCoords(int x, int y, int z, u32 mask)
{
    unsigned int h = ((unsigned int)x * 73856093) ^
                     ((unsigned int)y * 19349663) ^
                     ((unsigned int)z * 83492791);
    return h & mask;
}

void SpatialTable_Init(SpatialTable *table, u32 buckets, u32 capacity)
{
    table->size = buckets;
    table->mask = buckets - 1;
    table->head = malloc(sizeof(u32) * buckets);
    table->value = malloc(sizeof(u32) * capacity);
    table->next = malloc(sizeof(u32) * capacity);
    table->capacity = capacity;
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
    if (table->count >= table->capacity)
        return;

    u32 idx = table->count++;
    table->value[idx] = value;
    table->next[idx] = table->head[hash & table->mask];
    table->head[hash & table->mask] = idx;
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
    offsets[0] = 0;
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
            entry = table->next[entry];
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