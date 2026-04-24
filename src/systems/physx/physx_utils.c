#include <cglm/struct.h>

#include "sol_core.h"

void Spatial_Add(World *world, int id, CompBody *body)
{
    CompXform  *xform = &world->xforms[id];
    PhysxGroup *group = body->mass == 0 ? &world->spatial->staticGroup : &world->spatial->dynamicGroup;

    group->ents[id].id = id;
    group->entCount++;

    if (body->shape == SHAPE3_MOD)
    {
        SolModel *model    = world->models[id].model;
        u32       oldCount = group->triCount;
        u32       newCount = oldCount + model->tri_count;

        group->tris     = realloc(group->tris, sizeof(SolTri) * newCount);
        group->triCount = newCount;
        Transform_Tris_LocalToWorld(group->tris, oldCount, model, xform);

        group->ents[id].triIndexStart = oldCount;
        group->ents[id].triIndexCount = model->tri_count;
    }
}

void Fill_Dynamic_Table(World *world, int count, int *ents)
{
    SpatialTable *table = &world->spatial->dynamicGroup.table;
    SpatialTable_Clear(table);
    for (int i = 0; i < count; i++)
    {
        u32   id   = ents[i];
        vec3s pos  = world->xforms[id].pos;
        int   ix   = (int)floorf(pos.x / SPATIAL_DYNAMIC_CELL_SIZE);
        int   iy   = (int)floorf(pos.y / SPATIAL_DYNAMIC_CELL_SIZE);
        int   iz   = (int)floorf(pos.z / SPATIAL_DYNAMIC_CELL_SIZE);
        u32   hash = hash_coords(ix, iy, iz) & (table->size - 1);
        SpatialTable_Insert(table, hash, id);
    }
}

void Physx_Ground_Trace(World *world, CompBody *body, CompXform *xform)
{
    body->grounded = 0;
    SolRayResult result =
        Sol_Raycast(world, (SolRay){.pos = xform->pos, .dir = (vec3s){0, -1, 0}, .dist = 1.5f});
    if (result.hit)
    {
        if (result.norm.y > 0.5f)
            body->grounded = 1;
    }
}

SolCollision Collisions_Static_Hashed(PhysxGroup *group, CompBody *body, CompXform *xform, ResolveShapeTri resolver)
{
    int          checks = 0;
    SpatialCell  cell   = Spatial_Cell_Get(xform->pos, group->table.cellSize);
    SolCollision col    = {0};
    for (int c = 0; c < 27; c++)
    {
        u32 entry = group->table.head[cell.neighborHashes[c] & (group->table.size - 1)];
        while (entry != SPATIAL_NULL)
        {
            if (checks > 0x1ff)
                break;
            SolTri *tri = &group->tris[group->table.value[entry]];
            col         = resolver(body, xform, tri);
            entry       = group->table.next[entry];
        }
    }
    return col;
}

void Spatial_Add_Model(PhysxGroup *triGroup, int id, SolModel *model, CompXform *xform, bool hash)
{
    u32 oldCount = triGroup->triCount;
    u32 newCount = oldCount + model->tri_count;

    triGroup->tris     = realloc(triGroup->tris, sizeof(SolTri) * newCount);
    triGroup->triCount = newCount;
    Transform_Tris_LocalToWorld(triGroup->tris, oldCount, model, xform);

    triGroup->ents[id].triIndexStart = oldCount;
    triGroup->ents[id].triIndexCount = model->tri_count;
    triGroup->ents[id].id            = id;
    triGroup->entCount++;

    if (hash)
    {
        SpatialTable_Clear(&triGroup->table);
        Spatial_Hash_Tris(triGroup);
    }
}

void Transform_Tris_LocalToWorld(SolTri *group, int offset, SolModel *model, CompXform *xform)
{
    mat3s rot = glms_quat_mat3(xform->quat);
    for (int i = 0; i < model->tri_count; i++)
    {
        SolTri  src = model->tris[i];
        SolTri *dst = &group[offset + i];

        dst->a = glms_vec3_add(glms_mat3_mulv(rot, glms_vec3_mul(src.a, xform->scale)), xform->pos);
        dst->b = glms_vec3_add(glms_mat3_mulv(rot, glms_vec3_mul(src.b, xform->scale)), xform->pos);
        dst->c = glms_vec3_add(glms_mat3_mulv(rot, glms_vec3_mul(src.c, xform->scale)), xform->pos);

        // Recompute derived data in world space
        vec3s e1    = glms_vec3_sub(dst->b, dst->a);
        vec3s e2    = glms_vec3_sub(dst->c, dst->a);
        vec3s cross = glms_vec3_cross(e1, e2);
        float len   = glms_vec3_norm(cross);
        dst->normal = len > 0.00001f ? glms_vec3_scale(cross, 1.0f / len) : (vec3s){0, 1, 0};

        dst->center = glms_vec3_scale(glms_vec3_add(glms_vec3_add(dst->a, dst->b), dst->c), 1.0f / 3.0f);

        float da    = glms_vec3_norm(glms_vec3_sub(dst->a, dst->center));
        float db    = glms_vec3_norm(glms_vec3_sub(dst->b, dst->center));
        float dc    = glms_vec3_norm(glms_vec3_sub(dst->c, dst->center));
        dst->bounds = fmaxf(da, fmaxf(db, dc));
    }
}

void Physx_Mass_Set(World *world, int id, float mass)
{
    world->bodies[id].mass = mass;
    if (mass == 0.0f)
        if (world->bodies[id].shape == SHAPE3_MOD)
            Spatial_Add_Model(&world->spatial->staticGroup, id, world->models[id].model, &world->xforms[id], false);
}

void Spatial_Hash_Tris(PhysxGroup *group)
{
    SpatialTable *table    = &group->table;
    SolTri       *t        = group->tris;
    float         cellSize = table->cellSize;
    for (int i = 0; i < group->triCount; i++)
    {
        float minX = fminf(t[i].a.x, fminf(t[i].b.x, t[i].c.x));
        float maxX = fmaxf(t[i].a.x, fmaxf(t[i].b.x, t[i].c.x));
        float minY = fminf(t[i].a.y, fminf(t[i].b.y, t[i].c.y));
        float maxY = fmaxf(t[i].a.y, fmaxf(t[i].b.y, t[i].c.y));
        float minZ = fminf(t[i].a.z, fminf(t[i].b.z, t[i].c.z));
        float maxZ = fmaxf(t[i].a.z, fmaxf(t[i].b.z, t[i].c.z));

        int x0 = (int)floorf(minX / cellSize);
        int x1 = (int)floorf(maxX / cellSize);
        int y0 = (int)floorf(minY / cellSize);
        int y1 = (int)floorf(maxY / cellSize);
        int z0 = (int)floorf(minZ / cellSize);
        int z1 = (int)floorf(maxZ / cellSize);

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++)
                    SpatialTable_Insert(table, hash_coords(x, y, z) & (table->size - 1), i);
    }
}

SubstepData Substep_Get(CompBody *body, float fdt)
{
    SubstepData substep_data = {0};

    float speed    = glms_vec3_norm(body->vel);
    float stepDist = body->radius * 0.9f;
    u8    substeps = (int)ceilf(speed * fdt / stepDist);
    if (substeps < 1)
        substeps = 1;
    if (substeps > 8)
        substeps = 8;

    substep_data.substeps = substeps;
    substep_data.sub_dt   = fdt / substeps;

    return substep_data;
}

void Physx_Grid_Static_Rebuild(PhysxGroup *group)
{
    SpatialGrid *grid = &group->grid;
    if (!grid->offsets && group->triCount > 0)
    {
        vec3s min = {1e9f, 1e9f, 1e9f};
        vec3s max = {-1e9f, -1e9f, -1e9f};
        for (u32 t = 0; t < group->triCount; t++)
        {
            SolTri *tri = &group->tris[t];
            min.x       = fminf(min.x, fminf(tri->a.x, fminf(tri->b.x, tri->c.x)));
            min.y       = fminf(min.y, fminf(tri->a.y, fminf(tri->b.y, tri->c.y)));
            min.z       = fminf(min.z, fminf(tri->a.z, fminf(tri->b.z, tri->c.z)));
            max.x       = fmaxf(max.x, fmaxf(tri->a.x, fmaxf(tri->b.x, tri->c.x)));
            max.y       = fmaxf(max.y, fmaxf(tri->a.y, fmaxf(tri->b.y, tri->c.y)));
            max.z       = fmaxf(max.z, fmaxf(tri->a.z, fmaxf(tri->b.z, tri->c.z)));
        }
        Physx_Grid_Static_Build(group, min, max, 1.0f);
        grid->built_count = group->triCount;
    }
    else if (grid->built_count != group->triCount)
    {
        Physx_Grid_Static_Build(group, grid->min, grid->max, grid->cellSize);
        grid->built_count = group->triCount;
    }
}

SolCollision Collisions_Dynamic_Hashed(World *world, int id, CompBody *body, CompXform *xform)
{
    WorldPhysx  *ws   = world->spatial;
    SpatialCell  cell = Spatial_Cell_Get(xform->pos, SPATIAL_DYNAMIC_CELL_SIZE);
    SolCollision col;
    for (int n = 0; n < 27; n++)
    {
        u32 entry = ws->dynamicGroup.table.head[cell.neighborHashes[n] & (ws->dynamicGroup.table.size - 1)];
        while (entry != SPATIAL_NULL)
        {
            u32 otherID = ws->dynamicGroup.table.value[entry];
            if (id < otherID)
            {
                CompBody        *other_body  = &world->bodies[otherID];
                CompXform       *other_xform = &world->xforms[otherID];
                ResolveShapePair resolver    = shape_pair_resolvers[body->shape][other_body->shape];

                if (resolver)
                    col = resolver(body, xform, other_body, other_xform);
            }
            entry = ws->dynamicGroup.table.next[entry];
        }
    }
    return col;
}

SolCollision Collisions_Dynamic_Grid(World *world, int id, CompBody *body, CompXform *xform)
{
    SolCollision col   = {0};
    PhysxGroup  *group = &world->spatial->dynamicGroup;
    SpatialGrid *grid  = &group->grid;

    int checks = 0;
    int ix     = (int)floorf((xform->pos.x - grid->min.x) / grid->cellSize);
    int iy     = (int)floorf((xform->pos.y - grid->min.y) / grid->cellSize);
    int iz     = (int)floorf((xform->pos.z - grid->min.z) / grid->cellSize);

    for (int ox = -1; ox <= 1; ox++)
        for (int oy = -1; oy <= 1; oy++)
            for (int oz = -1; oz <= 1; oz++)
            {
                int cx = ix + ox, cy = iy + oy, cz = iz + oz;
                if (cx < 0 || cx >= grid->dims.x || cy < 0 || cy >= grid->dims.y || cz < 0 || cz >= grid->dims.z)
                    continue;

                u32 cell  = cx + cy * grid->dims.x + cz * grid->dims.x * grid->dims.y;
                u32 start = grid->offsets[cell];
                u32 end   = grid->offsets[cell + 1];

                for (u32 e = start; e < end; e++)
                {
                    if (++checks > 1000)
                        return col;

                    int        otherId    = group->ents[grid->values[e]].id;
                    CompBody  *otherBody  = &world->bodies[otherId];
                    CompXform *otherXform = &world->xforms[otherId];

                    ResolveShapePair resolver = shape_pair_resolvers[body->shape][otherBody->shape];
                    col                       = resolver(body, xform, otherBody, otherXform);
                }
            }
    return col;
}

SolRayResult Sol_RaycastD(World *world, SolRay ray, float debugDuration)
{
    SolRayResult result = Sol_Raycast(world, ray);
    Sol_World_Line_Add(world, ray.pos, result.pos, (vec3s){1, 0, 0}, (vec3s){1, 0, 0}, debugDuration);
    if (result.hit)
        Sol_World_Line_Add(world, result.pos,
                           glms_vec3_add(result.pos, glms_vec3_scale(ray.dir, ray.dist - result.dist)),
                           (vec3s){0, 1, 0}, (vec3s){0, 1, 0}, debugDuration);
    return result;
}

SolRayResult Raycast_Static_Grid(PhysxGroup *group, SolRay ray)
{
    SolRayResult result = {0};
    result.dist         = ray.dist;
    SpatialGrid *grid   = &group->grid;
    if (!grid->offsets)
        return result;

    vec3s pos  = ray.pos;
    vec3s dir  = ray.dir;
    float dist = ray.dist;

    // Current cell
    int ix = (int)floorf((pos.x - grid->min.x) / grid->cellSize);
    int iy = (int)floorf((pos.y - grid->min.y) / grid->cellSize);
    int iz = (int)floorf((pos.z - grid->min.z) / grid->cellSize);

    // If pos is outside grid, reject (or you could march the forward to grid
    // entry — skipping that for simplicity)
    if (ix < 0 || ix >= grid->dims.x || iy < 0 || iy >= grid->dims.y || iz < 0 || iz >= grid->dims.z)
        return result;

    // Direction signs
    int stepX = dir.x > 0 ? 1 : -1;
    int stepY = dir.y > 0 ? 1 : -1;
    int stepZ = dir.z > 0 ? 1 : -1;

    // Distance to next boundary along each axis
    // "Next boundary" depends on step direction
    float nextX = grid->min.x + (ix + (stepX > 0 ? 1 : 0)) * grid->cellSize;
    float nextY = grid->min.y + (iy + (stepY > 0 ? 1 : 0)) * grid->cellSize;
    float nextZ = grid->min.z + (iz + (stepZ > 0 ? 1 : 0)) * grid->cellSize;

    // t values where reaches those boundaries
    float tMaxX = (fabsf(dir.x) > 1e-8f) ? (nextX - pos.x) / dir.x : INFINITY;
    float tMaxY = (fabsf(dir.y) > 1e-8f) ? (nextY - pos.y) / dir.y : INFINITY;
    float tMaxZ = (fabsf(dir.z) > 1e-8f) ? (nextZ - pos.z) / dir.z : INFINITY;

    // t hitDist per cell along each axis
    float tDeltaX = (fabsf(dir.x) > 1e-8f) ? grid->cellSize / fabsf(dir.x) : INFINITY;
    float tDeltaY = (fabsf(dir.y) > 1e-8f) ? grid->cellSize / fabsf(dir.y) : INFINITY;
    float tDeltaZ = (fabsf(dir.z) > 1e-8f) ? grid->cellSize / fabsf(dir.z) : INFINITY;

    while (1)
    {
        // Test triangles in current cell
        u32 cellIdx = ix + iy * grid->dims.x + iz * grid->dims.x * grid->dims.y;
        u32 start   = grid->offsets[cellIdx];
        u32 end     = grid->offsets[cellIdx + 1];

        // The hitDist along the ray at which we leave this cell
        float tCellExit = fminf(tMaxX, fminf(tMaxY, tMaxZ));

        for (u32 e = start; e < end; e++)
        {
            SolTri *tri = &group->tris[grid->values[e]];
            vec3s   normal;
            float   t = Ray_Tri_Test(pos, dir, tri, &normal);

            if (t > 0 && t <= result.dist)
            {
                result.dist     = t;
                result.hit      = true;
                result.dist     = t;
                result.norm     = normal;
                result.triIndex = grid->values[e];
                result.pos      = glms_vec3_add(pos, glms_vec3_scale(dir, t));
            }
        }

        // If we found a hit within this cell's span, it's the nearest
        if (result.hit && result.dist <= tCellExit)
            break;

        float tCellEntry;
        // Advance to next cell along the axis with smallest tMax
        if (tMaxX < tMaxY && tMaxX < tMaxZ)
        {
            tCellEntry = tMaxX;
            ix += stepX;
            tMaxX += tDeltaX;
        }
        else if (tMaxY < tMaxZ)
        {
            tCellEntry = tMaxY;
            iy += stepY;
            tMaxY += tDeltaY;
        }
        else
        {
            tCellEntry = tMaxZ;
            iz += stepZ;
            tMaxZ += tDeltaZ;
        }

        if (tCellEntry > result.dist)
            break;

        // Exited grid bounds?
        if (ix < 0 || ix >= grid->dims.x || iy < 0 || iy >= grid->dims.y || iz < 0 || iz >= grid->dims.z)
            break;
    }

    return result;
}

inline float Ray_Tri_Test(vec3s origin, vec3s dir, SolTri *tri, vec3s *outNormal)
{
    vec3s edge1 = glms_vec3_sub(tri->b, tri->a);
    vec3s edge2 = glms_vec3_sub(tri->c, tri->a);
    vec3s h     = glms_vec3_cross(dir, edge2);
    float a     = glms_vec3_dot(edge1, h);

    if (fabsf(a) < FLOATING_EPSILON)
        return -1.0f; // ray parallel to triangle

    float f = 1.0f / a;
    vec3s s = glms_vec3_sub(origin, tri->a);
    float u = f * glms_vec3_dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return -1.0f;

    vec3s q = glms_vec3_cross(s, edge1);
    float v = f * glms_vec3_dot(dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return -1.0f;

    float t = f * glms_vec3_dot(edge2, q);
    if (t < FLOATING_EPSILON)
        return -1.0f; // behind origin or too close

    *outNormal = tri->normal;
    return t;
}

SolCollision Collisions_Static_Grid(PhysxGroup *group, CompBody *body, CompXform *xform, ResolveShapeTri resolver)
{
    SolCollision col  = {0};
    SpatialGrid *grid = &group->grid;

    int checks = 0;
    int ix     = (int)floorf((xform->pos.x - grid->min.x) / grid->cellSize);
    int iy     = (int)floorf((xform->pos.y - grid->min.y) / grid->cellSize);
    int iz     = (int)floorf((xform->pos.z - grid->min.z) / grid->cellSize);

    for (int ox = -1; ox <= 1; ox++)
        for (int oy = -1; oy <= 1; oy++)
            for (int oz = -1; oz <= 1; oz++)
            {
                int cx = ix + ox, cy = iy + oy, cz = iz + oz;
                if (cx < 0 || cx >= grid->dims.x || cy < 0 || cy >= grid->dims.y || cz < 0 || cz >= grid->dims.z)
                    continue;

                u32 cell  = cx + cy * grid->dims.x + cz * grid->dims.x * grid->dims.y;
                u32 start = grid->offsets[cell];
                u32 end   = grid->offsets[cell + 1];

                for (u32 e = start; e < end; e++)
                {
                    if (++checks > 1000)
                        return col;

                    SolTri *tri = &group->tris[grid->values[e]];
                    col         = resolver(body, xform, tri);
                }
            }
    return col;
}

void collisions_grid_dynamic(CompBody *body, CompXform *xform, SubstepData substep_data, WorldPhysx *ws)
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

void Physx_Grid_Static_Build(PhysxGroup *group, vec3s min, vec3s max, float cell_size)
{
    SpatialGrid *grid = &group->grid;
    free(grid->offsets);
    free(grid->values);
    grid->offsets = NULL;
    grid->values  = NULL;

    grid->cellSize = cell_size;
    grid->min      = min;
    grid->max      = max;
    grid->dims.x   = (int)ceilf((max.x - min.x) / cell_size);
    grid->dims.y   = (int)ceilf((max.y - min.y) / cell_size);
    grid->dims.z   = (int)ceilf((max.z - min.z) / cell_size);

    u32  cellCount    = (u32)grid->dims.x * grid->dims.y * grid->dims.z;
    u32 *counts       = calloc(cellCount, sizeof(u32));
    u32  totalEntries = 0;

    for (u32 t = 0; t < group->triCount; t++)
    {
        SolTri *tri  = &group->tris[t];
        float   minX = fminf(tri->a.x, fminf(tri->b.x, tri->c.x));
        float   maxX = fmaxf(tri->a.x, fmaxf(tri->b.x, tri->c.x));
        float   minY = fminf(tri->a.y, fminf(tri->b.y, tri->c.y));
        float   maxY = fmaxf(tri->a.y, fmaxf(tri->b.y, tri->c.y));
        float   minZ = fminf(tri->a.z, fminf(tri->b.z, tri->c.z));
        float   maxZ = fmaxf(tri->a.z, fmaxf(tri->b.z, tri->c.z));

        int x0 = (int)floorf((minX - min.x) / cell_size);
        int x1 = (int)floorf((maxX - min.x) / cell_size);
        int y0 = (int)floorf((minY - min.y) / cell_size);
        int y1 = (int)floorf((maxY - min.y) / cell_size);
        int z0 = (int)floorf((minZ - min.z) / cell_size);
        int z1 = (int)floorf((maxZ - min.z) / cell_size);

        x0 = x0 < 0 ? 0 : x0;
        x1 = x1 >= grid->dims.x ? grid->dims.x - 1 : x1;
        y0 = y0 < 0 ? 0 : y0;
        y1 = y1 >= grid->dims.y ? grid->dims.y - 1 : y1;
        z0 = z0 < 0 ? 0 : z0;
        z1 = z1 >= grid->dims.z ? grid->dims.z - 1 : z1;

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++)
                {
                    counts[x + y * grid->dims.x + z * grid->dims.x * grid->dims.y]++;
                    totalEntries++;
                }
    }

    grid->offsets    = malloc((cellCount + 1) * sizeof(u32));
    grid->offsets[0] = 0;
    for (u32 i = 0; i < cellCount; i++)
        grid->offsets[i + 1] = grid->offsets[i] + counts[i];

    grid->values = malloc(totalEntries * sizeof(u32));
    u32 *cursor  = malloc(cellCount * sizeof(u32));
    memcpy(cursor, grid->offsets, cellCount * sizeof(u32));

    for (u32 t = 0; t < group->triCount; t++)
    {
        SolTri *tri  = &group->tris[t];
        float   minX = fminf(tri->a.x, fminf(tri->b.x, tri->c.x));
        float   maxX = fmaxf(tri->a.x, fmaxf(tri->b.x, tri->c.x));
        float   minY = fminf(tri->a.y, fminf(tri->b.y, tri->c.y));
        float   maxY = fmaxf(tri->a.y, fmaxf(tri->b.y, tri->c.y));
        float   minZ = fminf(tri->a.z, fminf(tri->b.z, tri->c.z));
        float   maxZ = fmaxf(tri->a.z, fmaxf(tri->b.z, tri->c.z));

        int x0 = (int)floorf((minX - min.x) / cell_size);
        int x1 = (int)floorf((maxX - min.x) / cell_size);
        int y0 = (int)floorf((minY - min.y) / cell_size);
        int y1 = (int)floorf((maxY - min.y) / cell_size);
        int z0 = (int)floorf((minZ - min.z) / cell_size);
        int z1 = (int)floorf((maxZ - min.z) / cell_size);

        x0 = x0 < 0 ? 0 : x0;
        x1 = x1 >= grid->dims.x ? grid->dims.x - 1 : x1;
        y0 = y0 < 0 ? 0 : y0;
        y1 = y1 >= grid->dims.y ? grid->dims.y - 1 : y1;
        z0 = z0 < 0 ? 0 : z0;
        z1 = z1 >= grid->dims.z ? grid->dims.z - 1 : z1;

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++)
                {
                    u32 cell                     = x + y * grid->dims.x + z * grid->dims.x * grid->dims.y;
                    grid->values[cursor[cell]++] = t;
                }
    }

    free(counts);
    free(cursor);

    u32 worst = 0;
    for (u32 i = 0; i < cellCount; i++)
    {
        u32 len = grid->offsets[i + 1] - grid->offsets[i];
        if (len > worst)
            worst = len;
    }
    printf("grid_static: %dx%dx%d cells, %u entries, worst cell: %u\n", grid->dims.x, grid->dims.y, grid->dims.z,
           totalEntries, worst);
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

SolCollision Collide_Y(CompXform *xform, CompBody *body)
{
    SolCollision result = {0};
    float        pen    = 0.0f - (xform->pos.y - body->height / 2.0f);
    if (pen > 0)
    {
        body->grounded = 1;
        body->airtime  = 0;

        xform->pos.y += pen;
        body->vel.y       = 0;
        result.didCollide = true;
        result.normal     = (vec3s){0, 1, 0};
        result.pos        = xform->pos;
        result.pos.y      = 0;
    }
    else
    {
        body->grounded = 0;
        body->airtime  = 1;
    }

    return result;
}

SolCollision collide_box_tri(CompBody *body, CompXform *xform, SolTri *tri)
{
    SolCollision col = {0};
    vec3s       *pos = &xform->pos;

    // TODO

    return col;
}

SolCollision Collide_Sphere_Tri(CompBody *body, CompXform *xform, SolTri *tri)
{
    SolCollision result   = {0};
    vec3s       *localPos = &xform->pos;

    vec3s closestP = ClosestPointOnTriangle(*localPos, tri->a, tri->b, tri->c);
    vec3s delta    = glms_vec3_sub(*localPos, closestP);
    float distSq   = glms_vec3_dot(delta, delta);

    float radiusSq = body->radius * body->radius;
    if (distSq >= radiusSq)
        return result;

    float dist = sqrtf(distSq);

    vec3s normal      = dist > 0.0001f ? glms_vec3_scale(delta, 1.0f / dist) : tri->normal;
    float penetration = body->radius - dist;
    *localPos         = glms_vec3_add(*localPos, glms_vec3_scale(normal, penetration));

    float velAlongNormal = glms_vec3_dot(body->vel, normal);
    if (velAlongNormal < 0)
    {
        body->vel = glms_vec3_sub(body->vel, glms_vec3_scale(normal, velAlongNormal * body->restitution));
    }

    result.didCollide = true;
    result.pos        = closestP;
    result.normal     = normal;
    result.vel        = body->vel;

    return result;
}

SolCollision Collide_Capsule_Tri(CompBody *body, CompXform *xform, SolTri *tri)
{
    SolCollision result     = {0};
    vec3s       *pos        = &xform->pos;
    float        halfHeight = body->height * 0.5f - body->radius;
    float        boundsSq   = tri->bounds * tri->bounds + halfHeight;
    float        distSq     = glms_vec3_norm2(glms_vec3_sub(*pos, tri->center));
    if (distSq > boundsSq)
        return result;

    vec3s a = *pos;
    a.y += halfHeight;
    vec3s b = *pos;
    b.y -= halfHeight;

    // Find closest point on triangle to the capsule's line segment
    // by testing the segment against the triangle
    vec3s seg = glms_vec3_sub(b, a);

    // Get closest point on triangle to several points along the segment
    // and find which gives minimum hitDist
    vec3s bestCapsulePoint = a;
    vec3s bestTriPoint     = ClosestPointOnTriangle(a, tri->a, tri->b, tri->c);
    float bestDistSq       = glms_vec3_norm2(glms_vec3_sub(a, bestTriPoint));

    // Project triangle vertices onto segment to find the best capsule point
    vec3s triVerts[3] = {tri->a, tri->b, tri->c};
    for (int i = 0; i < 3; i++)
    {
        vec3s ap = glms_vec3_sub(triVerts[i], a);
        float t  = glms_vec3_dot(ap, seg) / fmaxf(glms_vec3_dot(seg, seg), 1e-8f);
        t        = fmaxf(0.0f, fminf(1.0f, t));

        vec3s capsulePoint = glms_vec3_add(a, glms_vec3_scale(seg, t));
        vec3s triPoint     = ClosestPointOnTriangle(capsulePoint, tri->a, tri->b, tri->c);
        float distSq       = glms_vec3_norm2(glms_vec3_sub(capsulePoint, triPoint));

        if (distSq < bestDistSq)
        {
            bestDistSq       = distSq;
            bestCapsulePoint = capsulePoint;
            bestTriPoint     = triPoint;
        }
    }

    // Also check triangle edges against segment
    vec3s edgeStarts[3] = {tri->a, tri->b, tri->c};
    vec3s edgeEnds[3]   = {tri->b, tri->c, tri->a};
    for (int i = 0; i < 3; i++)
    {
        // Closest point between two line segments (capsule seg and triangle edge)
        vec3s d1 = seg;
        vec3s d2 = glms_vec3_sub(edgeEnds[i], edgeStarts[i]);
        vec3s r  = glms_vec3_sub(a, edgeStarts[i]);

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
        vec3s edgePoint    = glms_vec3_add(edgeStarts[i], glms_vec3_scale(d2, t));
        float distSq       = glms_vec3_norm2(glms_vec3_sub(capsulePoint, edgePoint));

        if (distSq < bestDistSq)
        {
            bestDistSq       = distSq;
            bestCapsulePoint = capsulePoint;
            bestTriPoint     = edgePoint;
        }
    }

    // Now resolve as sphere at bestCapsulePoint
    float radiusSq = body->radius * body->radius;
    if (bestDistSq >= radiusSq)
        return result;

    float dist = sqrtf(bestDistSq);

    vec3s normal =
        dist > 0.0001f ? glms_vec3_scale(glms_vec3_sub(bestCapsulePoint, bestTriPoint), 1.0f / dist) : tri->normal;
    float penetration = body->radius - dist;
    *pos              = glms_vec3_add(*pos, glms_vec3_scale(normal, penetration));

    float velAlongNormal = glms_vec3_dot(body->vel, normal);
    if (velAlongNormal < 0)
    {
        body->vel = glms_vec3_sub(body->vel, glms_vec3_scale(normal, velAlongNormal * body->restitution));
    }

    result.didCollide = true;
    result.pos        = bestTriPoint;
    result.normal     = normal;
    result.vel        = body->vel;

    return result;
}

SolCollision collide_sphere_rect(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform)
{
    SolCollision col = {0};
    vec3s       *pos = &aXform->pos;

    // TODO

    return col;
}

SolCollision collide_sphere_sphere(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform)
{
    SolCollision col = {0};

    vec3s delta           = glms_vec3_sub(aXform->pos, bXform->pos);
    float combined_radius = aBody->radius + bBody->radius;
    float dist_sq         = glms_vec3_dot(delta, delta);

    if (dist_sq >= (combined_radius * combined_radius) || dist_sq < 0.0001f)
        return col;

    float distance    = sqrt(dist_sq);
    float penetration = combined_radius - distance;
    vec3s normal      = glms_vec3_scale(delta, 1.0f / distance);

    float invMassA     = aBody->invMass;
    float invMassB     = bBody->invMass;
    float totalInvMass = invMassA + invMassB;

    if (totalInvMass <= 0.000001f)
        return col;

    float slack               = 0.01f;
    float correctionMagnitude = fmaxf(penetration - slack, 0.0f) / totalInvMass;
    vec3s correctionVector    = glms_vec3_scale(normal, correctionMagnitude);

    aXform->pos = glms_vec3_add(aXform->pos, glms_vec3_scale(correctionVector, invMassA));
    bXform->pos = glms_vec3_sub(bXform->pos, glms_vec3_scale(correctionVector, invMassB));

    vec3s relativeVel    = glms_vec3_sub(aBody->vel, bBody->vel);
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
