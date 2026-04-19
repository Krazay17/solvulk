#include <cglm/struct.h>
#include <omp.h>

#include "sol_core.h"
#include "internal_physx.h"

static ResolveShapeTri shapeTriResolvers[SHAPE3_CNT] = {
    [SHAPE3_SPH] = collide_sphere_tri,
    [SHAPE3_CAP] = collide_capsule_tri,
    [SHAPE3_BOX] = collide_box_tri,
};

static int ents[MAX_ENTS];

static SolProfiler prof_static = {.name = "Static"};
static SolProfiler prof_dynamic = {.name = "Dynamic"};
static int prof_frame = 0;

void Sol_System_Step_Physx_3d(World *world, double dt, double time)
{
    if (Sol_GetState()->fps < 30)
        return;
    float fdt = (float)dt;
    int required = HAS_BODY3 | HAS_XFORM;
    int i, j, k, l;
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

    Prof_Begin(&prof_static);
    // Static position resolution
#pragma omp parallel for if (count > 1000) schedule(dynamic, 16)
    for (i = 0; i < count; i++)
    {
        int id = ents[i];
        CompXform *xform = &world->xforms[id];
        CompBody *body = &world->bodies[id];
        if (body->mass == 0)
            continue;

        if (xform->pos.y < -15.0f)
        {
            xform->pos = (vec3s){0, 100, 0};
            continue;
        }

        body->grounded = 0;

        vec3s accel = SOL_PHYS_GRAV;
        accel = glms_vec3_add(accel, body->force);
        accel = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));

        collisions_grid_static(body, xform, ws, fdt);
    }
    Prof_End(&prof_static);

    //     // Build dynamic spatial table
    //     for (l = 0; l < count; l++)
    //     {
    //         int id = ents[l];
    //         vec3s pos = world->xforms[id].pos;
    //         int ix = (int)floorf(pos.x / SPATIAL_DYNAMIC_CELL_SIZE);
    //         int iy = (int)floorf(pos.y / SPATIAL_DYNAMIC_CELL_SIZE);
    //         int iz = (int)floorf(pos.z / SPATIAL_DYNAMIC_CELL_SIZE);
    //         u32 hash = hash_coords(ix, iy, iz) & SPATIAL_DYNAMIC_SIZE;
    //         SpatialTable_Insert(&world->worldSpatial.table_dynamic, hash, (u32)id);
    //     }

    //     Prof_Begin(&prof_dynamic);
    //     // 2. Dynamic position resolution
    // #pragma omp parallel for if (count > 1000) schedule(dynamic, 16)
    //     for (j = 0; j < count; j++)
    //     {
    //         int id = ents[j];
    //         CompBody *body = &world->bodies[id];
    //         CompXform *xform = &world->xforms[id];

    //         if (body->invMass <= 0.0f)
    //             continue;

    //         collisions_hash_dynamic(world, id, body, xform);
    //     }
    //     Prof_End(&prof_dynamic);

    //     prof_frame++;
    //     if (prof_frame % 200 == 0)
    //     {
    //         Prof_Print(&prof_static);
    //         Prof_Print(&prof_dynamic);
    //     }
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

void collisions_hash_static(CompBody *body, CompXform *xform, SubstepData substep_data, WorldSpatial *ws)
{
    int totalChecks = 0;
    for (int s = 0; s < substep_data.substeps; s++)
    {
        xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, substep_data.sub_dt));
        SpatialCell cell = spatial_cell_get(xform->pos, SPATIAL_STATIC_CELL_SIZE);
        for (int n = 0; n < 27; n++)
        {
            u32 entry = ws->table_static.head[cell.neighborHashes[n]];
            while (entry != SPATIAL_NULL)
            {
                if (++totalChecks > 512)
                    return;
                SolTri *tri = &ws->tris_static[ws->table_static.value[entry]];
                SolCollision col = collide_sphere_tri(body, xform, tri);
                if (col.didCollide && col.normal.y > 0.5f)
                    body->grounded = 1;
                entry = ws->table_static.next[entry];
            }
        }
    }
}

void collisions_hash_dynamic(World *world, int id, CompBody *body, CompXform *xform)
{
    WorldSpatial *ws = &world->worldSpatial;
    SpatialCell cell = spatial_cell_get(xform->pos, SPATIAL_DYNAMIC_CELL_SIZE);
    for (int n = 0; n < 27; n++)
    {
        u32 entry = ws->table_dynamic.head[cell.neighborHashes[n] & SPATIAL_DYNAMIC_SIZE];
        while (entry != SPATIAL_NULL)
        {
            u32 otherID = ws->table_dynamic.value[entry];
            if (id < otherID)
                collide_sphere_sphere(body, xform,
                                      &world->bodies[otherID],
                                      &world->xforms[otherID]);
            entry = ws->table_dynamic.next[entry];
        }
    }
}

void collisions_grid_static(CompBody *body, CompXform *xform, WorldSpatial *ws, float fdt)
{
    SubstepData substep_data = substep_get(body, fdt);
    ResolveShapeTri resolve = shapeTriResolvers[body->shape];
    SpatialGrid *grid = &ws->grid_static;
    if (!grid->offsets && ws->tris_static_count > 0)
    {
        // First build — pick default bounds from tris
        vec3s min = {1e9f, 1e9f, 1e9f};
        vec3s max = {-1e9f, -1e9f, -1e9f};
        for (u32 t = 0; t < ws->tris_static_count; t++)
        {
            SolTri *tri = &ws->tris_static[t];
            min.x = fminf(min.x, fminf(tri->a.x, fminf(tri->b.x, tri->c.x)));
            min.y = fminf(min.y, fminf(tri->a.y, fminf(tri->b.y, tri->c.y)));
            min.z = fminf(min.z, fminf(tri->a.z, fminf(tri->b.z, tri->c.z)));
            max.x = fmaxf(max.x, fmaxf(tri->a.x, fmaxf(tri->b.x, tri->c.x)));
            max.y = fmaxf(max.y, fmaxf(tri->a.y, fmaxf(tri->b.y, tri->c.y)));
            max.z = fmaxf(max.z, fmaxf(tri->a.z, fmaxf(tri->b.z, tri->c.z)));
        }
        physx_grid_build_static(ws, min, max, 2.0f);
    }
    if (grid->needs_build)
    {
        grid->needs_build = false;
        physx_grid_build_static(ws, grid->min, (vec3s){grid->gridX, grid->gridY, grid->gridZ}, grid->cellSize);
    }

    for (int s = 0; s < substep_data.substeps; s++)
    {
        xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, substep_data.sub_dt));

        int checks = 0;
        int ix = (int)floorf((xform->pos.x - grid->min.x) / grid->cellSize);
        int iy = (int)floorf((xform->pos.y - grid->min.y) / grid->cellSize);
        int iz = (int)floorf((xform->pos.z - grid->min.z) / grid->cellSize);

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

                        SolTri *tri = &ws->tris_static[grid->tris[e]];
                        SolCollision col = resolve(body, xform, tri);

                        if (col.didCollide && col.normal.y > 0.5f)
                            body->grounded = 1;
                    }
                }
    }
}

void collisions_grid_dynamic(CompBody *body, CompXform *xform, SubstepData substep_data, WorldSpatial *ws)
{
}

void spatial_hash_tris(SolTri *t, int count, SpatialTable *table, float cellSize)
{
    for (int i = 0; i < count; i++)
    {
        float minX = fminf(t->a.x, fminf(t->b.x, t->c.x));
        float maxX = fmaxf(t->a.x, fmaxf(t->b.x, t->c.x));
        float minY = fminf(t->a.y, fminf(t->b.y, t->c.y));
        float maxY = fmaxf(t->a.y, fmaxf(t->b.y, t->c.y));
        float minZ = fminf(t->a.z, fminf(t->b.z, t->c.z));
        float maxZ = fmaxf(t->a.z, fmaxf(t->b.z, t->c.z));

        int x0 = (int)floorf(minX / cellSize);
        int x1 = (int)floorf(maxX / cellSize);
        int y0 = (int)floorf(minY / cellSize);
        int y1 = (int)floorf(maxY / cellSize);
        int z0 = (int)floorf(minZ / cellSize);
        int z1 = (int)floorf(maxZ / cellSize);

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++)
                    SpatialTable_Insert(table, hash_coords(x, y, z), i);
    }
}

void physx_grid_build_static(WorldSpatial *ws, vec3s min, vec3s max, float cell_size)
{
    SpatialGrid *grid = &ws->grid_static;
    grid->cellSize = cell_size;
    grid->min = min;
    grid->gridX = (int)ceilf((max.x - min.x) / cell_size) + 1;
    grid->gridY = (int)ceilf((max.y - min.y) / cell_size) + 1;
    grid->gridZ = (int)ceilf((max.z - min.z) / cell_size) + 1;
    u32 cellCount = grid->gridX * grid->gridY * grid->gridZ;

    // 1. Count entries per cell
    u32 *counts = calloc(cellCount, sizeof(u32));
    u32 totalEntries = 0;

    for (u32 t = 0; t < ws->tris_static_count; t++)
    {
        SolTri *tri = &ws->tris_static[t];
        float minX = fminf(tri->a.x, fminf(tri->b.x, tri->c.x));
        float maxX = fmaxf(tri->a.x, fmaxf(tri->b.x, tri->c.x));
        float minY = fminf(tri->a.y, fminf(tri->b.y, tri->c.y));
        float maxY = fmaxf(tri->a.y, fmaxf(tri->b.y, tri->c.y));
        float minZ = fminf(tri->a.z, fminf(tri->b.z, tri->c.z));
        float maxZ = fmaxf(tri->a.z, fmaxf(tri->b.z, tri->c.z));

        int x0 = (int)floorf((minX - min.x) / cell_size);
        int x1 = (int)floorf((maxX - min.x) / cell_size);
        int y0 = (int)floorf((minY - min.y) / cell_size);
        int y1 = (int)floorf((maxY - min.y) / cell_size);
        int z0 = (int)floorf((minZ - min.z) / cell_size);
        int z1 = (int)floorf((maxZ - min.z) / cell_size);

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

    for (u32 t = 0; t < ws->tris_static_count; t++)
    {
        SolTri *tri = &ws->tris_static[t];
        float minX = fminf(tri->a.x, fminf(tri->b.x, tri->c.x));
        float maxX = fmaxf(tri->a.x, fmaxf(tri->b.x, tri->c.x));
        float minY = fminf(tri->a.y, fminf(tri->b.y, tri->c.y));
        float maxY = fmaxf(tri->a.y, fmaxf(tri->b.y, tri->c.y));
        float minZ = fminf(tri->a.z, fminf(tri->b.z, tri->c.z));
        float maxZ = fmaxf(tri->a.z, fmaxf(tri->b.z, tri->c.z));

        int x0 = (int)floorf((minX - min.x) / cell_size);
        int x1 = (int)floorf((maxX - min.x) / cell_size);
        int y0 = (int)floorf((minY - min.y) / cell_size);
        int y1 = (int)floorf((maxY - min.y) / cell_size);
        int z0 = (int)floorf((minZ - min.z) / cell_size);
        int z1 = (int)floorf((maxZ - min.z) / cell_size);

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
    printf("grid_static: %dx%dx%d cells, %u entries, worst cell: %u\n",
           grid->gridX, grid->gridY, grid->gridZ, totalEntries, worst);
}

void spatial_static_add_model(WorldSpatial *ws, SolModel *model, CompXform *xform)
{
    u32 old_count = ws->tris_static_count;
    u32 new_count = old_count + model->tri_count;

    ws->tris_static = realloc(ws->tris_static, sizeof(SolTri) * new_count);
    ws->tris_static_count = new_count;

    mat3s rot = glms_quat_mat3(xform->quat);

    for (u32 t = 0; t < model->tri_count; t++)
    {
        SolTri src = model->tris[t];
        SolTri *dst = &ws->tris_static[old_count + t];

        // Transform: scale → rotate → translate
        dst->a = glms_vec3_add(glms_mat3_mulv(rot, glms_vec3_mul(src.a, xform->scale)), xform->pos);
        dst->b = glms_vec3_add(glms_mat3_mulv(rot, glms_vec3_mul(src.b, xform->scale)), xform->pos);
        dst->c = glms_vec3_add(glms_mat3_mulv(rot, glms_vec3_mul(src.c, xform->scale)), xform->pos);

        // Recompute derived data in world space
        vec3s e1 = glms_vec3_sub(dst->b, dst->a);
        vec3s e2 = glms_vec3_sub(dst->c, dst->a);
        vec3s cross = glms_vec3_cross(e1, e2);
        float len = glms_vec3_norm(cross);
        dst->normal = len > 0.00001f
                          ? glms_vec3_scale(cross, 1.0f / len)
                          : (vec3s){0, 1, 0};

        dst->center = glms_vec3_scale(
            glms_vec3_add(glms_vec3_add(dst->a, dst->b), dst->c), 1.0f / 3.0f);

        float da = glms_vec3_norm(glms_vec3_sub(dst->a, dst->center));
        float db = glms_vec3_norm(glms_vec3_sub(dst->b, dst->center));
        float dc = glms_vec3_norm(glms_vec3_sub(dst->c, dst->center));
        dst->boundRadius = fmaxf(da, fmaxf(db, dc));
    }
    ws->grid_static.needs_build = true;
}

SpatialCell spatial_cell_get(vec3s pos, float cellSize)
{
    SpatialCell cell;
    cell.ix = (int)floorf(pos.x / cellSize);
    cell.iy = (int)floorf(pos.y / cellSize);
    cell.iz = (int)floorf(pos.z / cellSize);

    int n = 0;
    for (int ox = -1; ox <= 1; ox++)
        for (int oy = -1; oy <= 1; oy++)
            for (int oz = -1; oz <= 1; oz++)
                cell.neighborHashes[n++] = hash_coords(
                    cell.ix + ox, cell.iy + oy, cell.iz + oz);
    return cell;
}

void SpatialTable_Init(SpatialTable *table, u32 buckets, u32 capacity)
{
    table->size = buckets;
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
    table->next[idx] = table->head[hash];
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

SolCollision collide_y0(CompXform *xform, CompBody *body)
{
    SolCollision result = {0};
    float pen = 0.0f - (xform->pos.y - body->height / 2.0f);
    if (pen > 0)
    {
        body->grounded = 1;
        body->airtime = 0;

        xform->pos.y += pen;
        body->vel.y = 0;
        result.didCollide = true;
        result.normal = (vec3s){0, 1, 0};
        result.pos = xform->pos;
        result.pos.y = 0;
    }
    else
    {
        body->grounded = 0;
        body->airtime = 1;
    }

    return result;
}

SolCollision collide_box_tri(CompBody *body, CompXform *xform, SolTri *tri)
{
    SolCollision col = {0};
    vec3s *pos = &xform->pos;

    // TODO

    return col;
}

SolCollision collide_sphere_tri(CompBody *body, CompXform *xform, SolTri *tri)
{
    SolCollision result = {0};
    vec3s *localPos = &xform->pos;

    // Bounding sphere early-out
    float dx = localPos->x - tri->center.x;
    float dy = localPos->y - tri->center.y;
    float dz = localPos->z - tri->center.z;
    float maxDist = body->radius + tri->boundRadius;
    if (dx * dx + dy * dy + dz * dz > maxDist * maxDist)
        return result;

    vec3s closestP = ClosestPointOnTriangle(*localPos, tri->a, tri->b, tri->c);
    vec3s delta = glms_vec3_sub(*localPos, closestP);
    float distSq = glms_vec3_dot(delta, delta);

    float radiusSq = body->radius * body->radius;
    if (distSq >= radiusSq)
        return result;

    float dist = sqrtf(distSq);

    vec3s normal = dist > 0.0001f
                       ? glms_vec3_scale(delta, 1.0f / dist)
                       : tri->normal;
    float penetration = body->radius - dist;
    *localPos = glms_vec3_add(*localPos, glms_vec3_scale(normal, penetration));

    float velAlongNormal = glms_vec3_dot(body->vel, normal);
    if (velAlongNormal < 0)
    {
        body->vel = glms_vec3_sub(
            body->vel,
            glms_vec3_scale(normal, velAlongNormal * body->restitution));
    }

    result.didCollide = true;
    result.pos = closestP;
    result.normal = normal;
    result.vel = body->vel;

    return result;
}

SolCollision collide_capsule_tri(CompBody *body, CompXform *xform, SolTri *tri)
{
    SolCollision result = {0};
    vec3s *pos = &xform->pos;

    // Bounding sphere early-out (use height to expand check)
    float dx = pos->x - tri->center.x;
    float dy = pos->y - tri->center.y;
    float dz = pos->z - tri->center.z;
    float maxDist = body->radius + body->height * 0.5f + tri->boundRadius;
    if (dx * dx + dy * dy + dz * dz > maxDist * maxDist)
        return result;

    float halfHeight = body->height * 0.5f - body->radius;

    vec3s a = *pos;
    a.y += halfHeight;
    vec3s b = *pos;
    b.y -= halfHeight;

    // Find closest point on triangle to the capsule's line segment
    // by testing the segment against the triangle
    vec3s seg = glms_vec3_sub(b, a);

    // Get closest point on triangle to several points along the segment
    // and find which gives minimum distance
    vec3s bestCapsulePoint = a;
    vec3s bestTriPoint = ClosestPointOnTriangle(a, tri->a, tri->b, tri->c);
    float bestDistSq = glms_vec3_norm2(glms_vec3_sub(a, bestTriPoint));

    // Project triangle vertices onto segment to find the best capsule point
    vec3s triVerts[3] = {tri->a, tri->b, tri->c};
    for (int i = 0; i < 3; i++)
    {
        vec3s ap = glms_vec3_sub(triVerts[i], a);
        float t = glms_vec3_dot(ap, seg) / fmaxf(glms_vec3_dot(seg, seg), 1e-8f);
        t = fmaxf(0.0f, fminf(1.0f, t));

        vec3s capsulePoint = glms_vec3_add(a, glms_vec3_scale(seg, t));
        vec3s triPoint = ClosestPointOnTriangle(capsulePoint, tri->a, tri->b, tri->c);
        float distSq = glms_vec3_norm2(glms_vec3_sub(capsulePoint, triPoint));

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestCapsulePoint = capsulePoint;
            bestTriPoint = triPoint;
        }
    }

    // Also check triangle edges against segment
    vec3s edgeStarts[3] = {tri->a, tri->b, tri->c};
    vec3s edgeEnds[3] = {tri->b, tri->c, tri->a};
    for (int i = 0; i < 3; i++)
    {
        // Closest point between two line segments (capsule seg and triangle edge)
        vec3s d1 = seg;
        vec3s d2 = glms_vec3_sub(edgeEnds[i], edgeStarts[i]);
        vec3s r = glms_vec3_sub(a, edgeStarts[i]);

        float aa = glms_vec3_dot(d1, d1);
        float bb = glms_vec3_dot(d2, d2);
        float ab = glms_vec3_dot(d1, d2);
        float ra = glms_vec3_dot(r, d1);
        float rb = glms_vec3_dot(r, d2);

        float denom = aa * bb - ab * ab;
        float s, t;

        if (denom < 1e-8f)
        {
            s = 0.0f;
            t = rb / fmaxf(bb, 1e-8f);
        }
        else
        {
            s = (ra * bb - rb * ab) / denom;
            t = (ra * ab - rb * aa) / -denom;
        }

        s = fmaxf(0.0f, fminf(1.0f, s));
        t = fmaxf(0.0f, fminf(1.0f, t));

        // Recompute closest points after clamping
        vec3s capsulePoint = glms_vec3_add(a, glms_vec3_scale(d1, s));
        vec3s edgePoint = glms_vec3_add(edgeStarts[i], glms_vec3_scale(d2, t));
        float distSq = glms_vec3_norm2(glms_vec3_sub(capsulePoint, edgePoint));

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestCapsulePoint = capsulePoint;
            bestTriPoint = edgePoint;
        }
    }

    // Now resolve as sphere at bestCapsulePoint
    float radiusSq = body->radius * body->radius;
    if (bestDistSq >= radiusSq)
        return result;

    float dist = sqrtf(bestDistSq);

    vec3s normal = dist > 0.0001f
                       ? glms_vec3_scale(glms_vec3_sub(bestCapsulePoint, bestTriPoint), 1.0f / dist)
                       : tri->normal;
    float penetration = body->radius - dist;
    *pos = glms_vec3_add(*pos, glms_vec3_scale(normal, penetration));

    float velAlongNormal = glms_vec3_dot(body->vel, normal);
    if (velAlongNormal < 0)
    {
        body->vel = glms_vec3_sub(
            body->vel,
            glms_vec3_scale(normal, velAlongNormal * body->restitution));
    }

    result.didCollide = true;
    result.pos = bestTriPoint;
    result.normal = normal;
    result.vel = body->vel;

    return result;
}

SolCollision collide_sphere_rect(CompBody *aBody, CompXform *aXform,
                                 CompBody *bBody, CompXform *bXform)
{
    SolCollision col = {0};
    vec3s *pos = &aXform->pos;

    // TODO

    return col;
}

SolCollision collide_sphere_sphere(CompBody *aBody, CompXform *aXform,
                                   CompBody *bBody, CompXform *bXform)
{
    SolCollision col = {0};

    vec3s delta = glms_vec3_sub(aXform->pos, bXform->pos);
    float combined_radius = aBody->radius + bBody->radius;
    float dist_sq = glms_vec3_dot(delta, delta);

    if (dist_sq >= (combined_radius * combined_radius) || dist_sq < 0.0001f)
        return col;

    float distance = sqrt(dist_sq);
    float penetration = combined_radius - distance;
    vec3s normal = glms_vec3_scale(delta, 1.0f / distance);

    float invMassA = aBody->invMass;
    float invMassB = bBody->invMass;
    float totalInvMass = invMassA + invMassB;

    if (totalInvMass <= 0.000001f)
        return col;

    float slack = 0.01f;
    float correctionMagnitude = fmaxf(penetration - slack, 0.0f) / totalInvMass;
    vec3s correctionVector = glms_vec3_scale(normal, correctionMagnitude);

    aXform->pos = glms_vec3_add(aXform->pos, glms_vec3_scale(correctionVector, invMassA));
    bXform->pos = glms_vec3_sub(bXform->pos, glms_vec3_scale(correctionVector, invMassB));

    vec3s relativeVel = glms_vec3_sub(aBody->vel, bBody->vel);
    float velAlongNormal = glms_vec3_dot(relativeVel, normal);

    if (velAlongNormal > 0)
        return col;

    float e = fminf(aBody->restitution, bBody->restitution);
    float j = -(1.0f + e) * velAlongNormal;
    j /= totalInvMass;

    vec3s impulse = glms_vec3_scale(normal, j);

    aBody->vel = glms_vec3_add(aBody->vel, glms_vec3_scale(impulse, invMassA));
    bBody->vel = glms_vec3_sub(bBody->vel, glms_vec3_scale(impulse, invMassB));

    return col;
}
