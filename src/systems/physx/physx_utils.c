#include "physx.h"
#include "sol_core.h"
#include <omp.h>

#include "xform/xform.h"

ShapeTriTest shape_tri_test[SHAPE3_CNT] = {
    [SHAPE3_SPH] = Collide_Sphere_Tri,
    [SHAPE3_CAP] = Collide_Capsule_Tri,
    [SHAPE3_BOX] = Collide_Box_Tri,
};

ShapePairTest shape_pair_test[SHAPE3_CNT][SHAPE3_CNT] = {
    [SHAPE3_SPH][SHAPE3_SPH] = Collide_Sphere_Sphere,
    [SHAPE3_CAP][SHAPE3_CAP] = Collide_Sphere_Sphere,
    [SHAPE3_CAP][SHAPE3_SPH] = Collide_Sphere_Sphere,
    [SHAPE3_SPH][SHAPE3_CAP] = Collide_Sphere_Sphere,
};

RaycastTest ray_shape_test[SHAPE3_CNT] = {
    [SHAPE3_SPH] = Ray_Sphere_Test,
    [SHAPE3_CAP] = Ray_Capsule_Test,
    [SHAPE3_BOX] = NULL,
    [SHAPE3_MOD] = NULL,
};

void Spatial_Add(World *world, int id, CompBody *body)
{
    CompXform  *xform = &world->xforms[id];
    PhysxGroup *group = body->mass == 0 ? &world->spatial->staticGroup : &world->spatial->dynamicGroup;

    group->ents[id].id = id;
    group->entCount++;

    if (body->shape == SHAPE3_MOD)
    {
        SolModel *model = &Sol_Bank_Get()->models[Sol_Model_GetModelId(world, id)];
        printf("phys add model %d\n", model->vertex_count);
        // SolModel *model    = world->models[id].model;
        u32 oldCount = group->triCount;
        u32 newCount = oldCount + model->tri_count;

        if (newCount > group->capacity)
        {
            group->capacity = newCount * 2;
            group->tris     = realloc(group->tris, sizeof(SolTri) * group->capacity);
        }

        group->triCount = newCount;
        Transform_Tris_LocalToWorld(group->tris, id, oldCount, model, xform);

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
        u32   id  = ents[i];
        vec3s pos = world->xforms[id].pos;
        float r   = fmaxf(world->bodies[id].dims.x, world->bodies[id].dims.y * 0.5f);

        int x0 = (int)floorf((pos.x - r) / SPATIAL_DYNAMIC_CELL_SIZE);
        int x1 = (int)floorf((pos.x + r) / SPATIAL_DYNAMIC_CELL_SIZE);
        int y0 = (int)floorf((pos.y - r) / SPATIAL_DYNAMIC_CELL_SIZE);
        int y1 = (int)floorf((pos.y + r) / SPATIAL_DYNAMIC_CELL_SIZE);
        int z0 = (int)floorf((pos.z - r) / SPATIAL_DYNAMIC_CELL_SIZE);
        int z1 = (int)floorf((pos.z + r) / SPATIAL_DYNAMIC_CELL_SIZE);
        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++)
                {
                    u32 hash = hash_coords(x, y, z) & (table->size - 1);
                    SpatialTable_Insert(table, hash, id);
                }
    }
}

// SolContact Collisions_Static_Hashed(PhysxGroup *group, CompBody *body, CompXform *xform, ResolveShapeTri resolver)
// {
//     int         checks = 0;
//     SpatialCell cell   = Spatial_Cell_Get(xform->pos, group->table.cellSize);
//     SolContact  col    = {0};
//     for (int c = 0; c < 27; c++)
//     {
//         u32 entry = group->table.head[cell.neighborHashes[c] & (group->table.size - 1)];
//         while (entry != SPATIAL_NULL)
//         {
//             if (checks > 0x1ff)
//                 break;
//             SolTri *tri = &group->tris[group->table.value[entry]];
//             col         = resolver(body, xform, tri);
//             entry       = group->table.next[entry];
//         }
//     }
//     return col;
// }

void Spatial_Add_Model(PhysxGroup *triGroup, int id, SolModel *model, CompXform *xform, bool hash)
{
    u32 oldCount = triGroup->triCount;
    u32 newCount = oldCount + model->tri_count;

    triGroup->tris     = realloc(triGroup->tris, sizeof(SolTri) * newCount);
    triGroup->triCount = newCount;
    Transform_Tris_LocalToWorld(triGroup->tris, id, oldCount, model, xform);

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
static SolProfiler local_tris = {.name = "LocalTris"};
void               Transform_Tris_LocalToWorld(SolTri *group, int id, int offset, SolModel *model, CompXform *xform)
{
    Prof_Begin(&local_tris);
    mat3s rot = glms_quat_mat3(xform->quat);
    for (int i = 0; i < model->tri_count; i++)
    {
        SolTri  src = model->tris[i];
        SolTri *dst = &group[offset + i];
        dst->entId  = id;

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
    Prof_EndEz(&local_tris, false);
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
    float stepDist = body->dims.x * 0.9f;
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

void Collisions_Dynamic_Hashed(World *world, int id, CompBody *body, CompXform *xform)
{
    WorldPhysx *ws   = world->spatial;
    SpatialCell cell = Spatial_Cell_Get(xform->pos, SPATIAL_DYNAMIC_CELL_SIZE);
    SolContact  col  = {0};
    for (int n = 0; n < 27; n++)
    {
        u32 entry = ws->dynamicGroup.table.head[cell.neighborHashes[n] & (ws->dynamicGroup.table.size - 1)];
        while (entry != SPATIAL_NULL)
        {
            u32 otherID = ws->dynamicGroup.table.value[entry];
            if (id < otherID)
            {
                CompBody  *other_body  = &world->bodies[otherID];
                CompXform *other_xform = &world->xforms[otherID];
                SolContact result      = {0};
                if (shape_pair_test[body->shape][other_body->shape](body, xform, other_body, other_xform, &result))
                {
                    Resolve_Dynamic_Pair(body, xform, other_body, other_xform, &result);
                    // Sol_Event_Add(world, (EventDesc){.entA = id, .kind = EVENT_PARTICLE, .pos = result.point});
                }
            }
            entry = ws->dynamicGroup.table.next[entry];
        }
    }
}

// SolContact Collisions_Dynamic_Grid(World *world, int id, CompBody *body, CompXform *xform)
// {
// SolContact   col   = {0};
// PhysxGroup  *group = &world->spatial->dynamicGroup;
// SpatialGrid *grid  = &group->grid;

// int checks = 0;
// int ix     = (int)floorf((xform->pos.x - grid->min.x) / grid->cellSize);
// int iy     = (int)floorf((xform->pos.y - grid->min.y) / grid->cellSize);
// int iz     = (int)floorf((xform->pos.z - grid->min.z) / grid->cellSize);

// for (int ox = -1; ox <= 1; ox++)
//     for (int oy = -1; oy <= 1; oy++)
//         for (int oz = -1; oz <= 1; oz++)
//         {
//             int cx = ix + ox, cy = iy + oy, cz = iz + oz;
//             if (cx < 0 || cx >= grid->dims.x || cy < 0 || cy >= grid->dims.y || cz < 0 || cz >= grid->dims.z)
//                 continue;

//             u32 cell  = cx + cy * grid->dims.x + cz * grid->dims.x * grid->dims.y;
//             u32 start = grid->offsets[cell];
//             u32 end   = grid->offsets[cell + 1];

//             for (u32 e = start; e < end; e++)
//             {
//                 if (++checks > 1000)
//                     return col;

//                 int        otherId    = group->ents[grid->values[e]].id;
//                 CompBody  *otherBody  = &world->bodies[otherId];
//                 CompXform *otherXform = &world->xforms[otherId];

//                 ShapePairTest resolver = shape_pair_test[body->shape][otherBody->shape];
//                 col                    = resolver(body, xform, otherBody, otherXform);
//             }
//         }
// return col;
//}

float Ray_Sphere_Test(SolRay ray, CompXform *xform, CompBody *body, vec3s *outNormal)
{
    vec3s center = xform->pos;
    float radius = body->dims.x;

    vec3s oc = glms_vec3_sub(ray.pos, center);
    float b  = glms_vec3_dot(oc, ray.dir);
    float c  = glms_vec3_dot(oc, oc) - radius * radius;

    // Outside sphere and pointing away
    if (c > 0.0f && b > 0.0f)
        return -1.0f;

    float disc = b * b - c;
    if (disc < 0.0f)
        return -1.0f;

    float sq = sqrtf(disc);
    float t  = -b - sq;
    if (t < 0.0f)
        t = -b + sq; // origin inside sphere → use far hit
    if (t < 0.0f)
        return -1.0f;

    vec3s hit = glms_vec3_add(ray.pos, glms_vec3_scale(ray.dir, t));
    *outNormal =
        (radius > FLOATING_EPSILON) ? glms_vec3_scale(glms_vec3_sub(hit, center), 1.0f / radius) : (vec3s){{0, 1, 0}};
    return t;
}

float Ray_Capsule_Test(SolRay ray, CompXform *xform, CompBody *body, vec3s *outNormal)
{
    vec3s center = xform->pos;
    float radius = body->dims.x;
    float halfH  = body->dims.y * 0.5f - radius; // half of cylinder portion

    // Capsule axis: A (bottom hemisphere center) → B (top hemisphere center)
    vec3s A         = {{center.x, center.y - halfH, center.z}};
    vec3s B         = {{center.x, center.y + halfH, center.z}};
    vec3s axis      = glms_vec3_sub(B, A); // length = 2*halfH
    float axisLenSq = glms_vec3_dot(axis, axis);

    // Vector from A to ray origin
    vec3s oa = glms_vec3_sub(ray.pos, A);

    // Project ray direction and oa onto the axis
    float dDotAxis  = glms_vec3_dot(ray.dir, axis);
    float oaDotAxis = glms_vec3_dot(oa, axis);

    // Coefficients of quadratic for cylinder intersection
    // Standard infinite cylinder: a*t^2 + 2*b*t + c = 0
    float a = axisLenSq - dDotAxis * dDotAxis;
    float b = axisLenSq * glms_vec3_dot(oa, ray.dir) - oaDotAxis * dDotAxis;
    float c = axisLenSq * glms_vec3_dot(oa, oa) - oaDotAxis * oaDotAxis - radius * radius * axisLenSq;

    float bestT = INFINITY;

    // === Cylinder body ===
    // Ray parallel to axis (a ≈ 0) skips this — the hemispheres handle it.
    if (fabsf(a) > FLOATING_EPSILON)
    {
        float disc = b * b - a * c;
        if (disc >= 0.0f)
        {
            float sq = sqrtf(disc);
            float t  = (-b - sq) / a;
            if (t < 0.0f)
                t = (-b + sq) / a; // origin inside? try far hit

            if (t >= 0.0f)
            {
                // Check the hit lies between A and B along the axis
                float hitProj = oaDotAxis + t * dDotAxis;
                if (hitProj >= 0.0f && hitProj <= axisLenSq)
                {
                    bestT = t;

                    // Normal: from axis to hit, perpendicular to axis
                    vec3s hit       = glms_vec3_add(ray.pos, glms_vec3_scale(ray.dir, t));
                    vec3s axisPoint = glms_vec3_add(A, glms_vec3_scale(axis, hitProj / axisLenSq));
                    vec3s n         = glms_vec3_sub(hit, axisPoint);
                    float nLen      = glms_vec3_norm(n);
                    *outNormal      = (nLen > FLOATING_EPSILON) ? glms_vec3_scale(n, 1.0f / nLen) : (vec3s){{1, 0, 0}};
                }
            }
        }
    }

    // === Hemispheres (test both, keep closer hit) ===
    vec3s hemiCenters[2] = {A, B};
    for (int i = 0; i < 2; i++)
    {
        vec3s hc  = hemiCenters[i];
        vec3s oc  = glms_vec3_sub(ray.pos, hc);
        float hb  = glms_vec3_dot(oc, ray.dir);
        float hc2 = glms_vec3_dot(oc, oc) - radius * radius;

        if (hc2 > 0.0f && hb > 0.0f)
            continue;

        float disc = hb * hb - hc2;
        if (disc < 0.0f)
            continue;

        float sq = sqrtf(disc);
        float t  = -hb - sq;
        if (t < 0.0f)
            t = -hb + sq;
        if (t < 0.0f)
            continue;

        if (t >= bestT)
            continue;

        // Verify the hit is on the correct hemisphere, not in the cylinder zone.
        // Project the hit onto the axis. If A side: proj <= 0. If B side: proj >= axisLenSq.
        vec3s hit     = glms_vec3_add(ray.pos, glms_vec3_scale(ray.dir, t));
        float hitProj = glms_vec3_dot(glms_vec3_sub(hit, A), axis);

        bool onCorrectHemi = (i == 0) ? (hitProj <= 0.0f) : (hitProj >= axisLenSq);
        if (!onCorrectHemi)
            continue;

        bestT = t;
        *outNormal =
            (radius > FLOATING_EPSILON) ? glms_vec3_scale(glms_vec3_sub(hit, hc), 1.0f / radius) : (vec3s){{0, 1, 0}};
    }

    return (bestT == INFINITY) ? -1.0f : bestT;
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

void Resolve_Contact(CompBody *body, CompXform *xform, SolContact *hit)
{
    xform->pos           = vecAdd(xform->pos, vecSca(hit->normal, hit->penetration));
    float velAlongNormal = glms_vec3_dot(body->vel, hit->normal);
    if (velAlongNormal < 0)
    {
        float j   = -(1.0f + body->restitution) * velAlongNormal;
        body->vel = glms_vec3_add(body->vel, glms_vec3_scale(hit->normal, j));
    }
}

void Collisions_Static_Grid(PhysxGroup *group, CompBody *body, CompXform *xform, SolContact *hit)
{
    SpatialGrid *grid      = &group->grid;
    ShapeTriTest shapeTest = shape_tri_test[body->shape];
    int          checks    = 0;

    // Compute capsule's AABB extents in cell units
    float halfH = (body->shape == SHAPE3_CAP) ? (body->dims.y * 0.5f) : body->dims.x;
    float r     = body->dims.x;

    int x0 = (int)floorf((xform->pos.x - r - grid->min.x) / grid->cellSize);
    int x1 = (int)floorf((xform->pos.x + r - grid->min.x) / grid->cellSize);
    int y0 = (int)floorf((xform->pos.y - halfH - grid->min.y) / grid->cellSize);
    int y1 = (int)floorf((xform->pos.y + halfH - grid->min.y) / grid->cellSize);
    int z0 = (int)floorf((xform->pos.z - r - grid->min.z) / grid->cellSize);
    int z1 = (int)floorf((xform->pos.z + r - grid->min.z) / grid->cellSize);

    for (int cx = x0; cx <= x1; cx++)
        for (int cy = y0; cy <= y1; cy++)
            for (int cz = z0; cz <= z1; cz++)
            {
                if (cx < 0 || cx >= grid->dims.x || cy < 0 || cy >= grid->dims.y || cz < 0 || cz >= grid->dims.z)
                    continue;

                u32 cell  = cx + cy * grid->dims.x + cz * grid->dims.x * grid->dims.y;
                u32 start = grid->offsets[cell];
                u32 end   = grid->offsets[cell + 1];

                for (u32 e = start; e < end; e++)
                {
                    if (++checks > 1000)
                        return;
                    SolTri *tri = &group->tris[grid->values[e]];
                    if (shapeTest(body, xform, tri, hit))
                    {
                        Resolve_Contact(body, xform, hit);
                    }
                }
            }
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

    for (int t = 0; t < group->triCount; t++)
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

bool Collide_Y(CompXform *xform, CompBody *body, SolContact *hit)
{
    float pen = 0.0f - (xform->pos.y - body->dims.y / 2.0f);
    if (pen > 0)
    {
        body->grounded = 1;
        body->airtime  = 0;

        xform->pos.y += pen;
        body->vel.y     = 0;
        hit->didCollide = true;
        hit->normal     = (vec3s){0, 1, 0};
        hit->point      = xform->pos;
        hit->point.y    = 0;
    }
    else
    {
        body->grounded = 0;
        body->airtime  = 1;
    }

    return true;
}

bool Collide_Box_Tri(CompBody *body, CompXform *xform, SolTri *tri, SolContact *col)
{
    vec3s *pos = &xform->pos;

    // TODO

    return false;
}

bool Collide_Sphere_Tri(CompBody *body, CompXform *xform, SolTri *tri, SolContact *col)
{
    vec3s closestP = ClosestPointOnTriangle(xform->pos, tri->a, tri->b, tri->c);
    vec3s delta    = glms_vec3_sub(xform->pos, closestP);
    float distSq   = glms_vec3_dot(delta, delta);

    if (distSq >= body->dims.x * body->dims.x)
        return false;

    float dist        = sqrtf(distSq);
    vec3s normal      = dist > 0.0001f ? glms_vec3_scale(delta, 1.0f / dist) : tri->normal;
    float penetration = body->dims.x - dist;

    col->point       = closestP;
    col->penetration = penetration;
    col->normal      = normal;
    col->didCollide  = true;

    return true;
}

bool Collide_Capsule_Tri(CompBody *body, CompXform *xform, SolTri *tri, SolContact *hit)
{
    vec3s *pos             = &xform->pos;
    float  halfHeight      = body->dims.y * 0.5f - body->dims.x;
    float  capsuleMaxReach = halfHeight + body->dims.x;
    float  maxDist         = tri->bounds + capsuleMaxReach;
    float  boundsSq        = maxDist * maxDist;
    float  distSq          = glms_vec3_norm2(glms_vec3_sub(*pos, tri->center));

    // if (distSq > boundsSq)
    //     return false;

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
            s = (ab * rb - bb * ra) / denom;
            t = (aa * rb - ab * ra) / denom;
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
    float radiusSq = body->dims.x * body->dims.x;
    if (bestDistSq >= radiusSq)
        return false;

    float dist = sqrtf(bestDistSq);

    hit->normal =
        dist > 0.0001f ? glms_vec3_scale(glms_vec3_sub(bestCapsulePoint, bestTriPoint), 1.0f / dist) : tri->normal;
    hit->penetration = body->dims.x - dist;
    hit->didCollide  = true;
    hit->point       = bestTriPoint;

    return true;
}

bool Collide_Sphere_Box(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform, SolContact *hit)
{
    SolContact col = {0};
    vec3s     *pos = &aXform->pos;

    // TODO

    return true;
}

bool Collide_Sphere_Sphere(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform, SolContact *hit)
{
    vec3s delta     = glms_vec3_sub(aXform->pos, bXform->pos);
    float distSq    = glms_vec3_dot(delta, delta);
    float radiusSum = aBody->dims.x + bBody->dims.y;

    if (distSq >= (radiusSum * radiusSum) || distSq < 0.0001f)
        return false;

    float distance = sqrtf(distSq);

    hit->normal      = glms_vec3_scale(delta, 1.0f / distance);
    hit->penetration = radiusSum - distance;

    // Contact point is halfway between the two surfaces
    hit->point = glms_vec3_add(bXform->pos, glms_vec3_scale(hit->normal, bBody->dims.x));

    return true;
}

void Resolve_Dynamic_Pair(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform, SolContact *hit)
{
    float totalInvMass = aBody->invMass + bBody->invMass;
    if (totalInvMass <= 0.0f)
        return;

    // --- 1. Positional Correction (The "Push Out") ---
    // We use "slack" (also called a "slop") to prevent jittering when objects barely touch.
    float slack   = 0.01f;
    float percent = 0.8f; // How much of the penetration to fix per frame (0.2 to 0.8)

    float corrMag    = (fmaxf(hit->penetration - slack, 0.0f) / totalInvMass) * percent;
    vec3s correction = glms_vec3_scale(hit->normal, corrMag);

    aXform->pos = glms_vec3_add(aXform->pos, glms_vec3_scale(correction, aBody->invMass));
    bXform->pos = glms_vec3_sub(bXform->pos, glms_vec3_scale(correction, bBody->invMass));

    // --- 2. Velocity Resolution (The "Bounce") ---
    vec3s relativeVel    = glms_vec3_sub(aBody->vel, bBody->vel);
    float velAlongNormal = glms_vec3_dot(relativeVel, hit->normal);

    // If objects are already moving apart, don't apply an impulse
    if (velAlongNormal > 0)
        return;

    // Use the lower restitution of the two objects
    float e = fminf(aBody->restitution, bBody->restitution);

    // Standard Impulse Formula: j = -(1+e)v_rel / (invM_a + invM_b)
    float j = -(1.0f + e) * velAlongNormal;
    j /= totalInvMass;

    vec3s impulse = glms_vec3_scale(hit->normal, j);

    aBody->vel = glms_vec3_add(aBody->vel, glms_vec3_scale(impulse, aBody->invMass));
    bBody->vel = glms_vec3_sub(bBody->vel, glms_vec3_scale(impulse, bBody->invMass));
}

SolRayResult Sol_RaycastD(World *world, SolRay ray, float debugDuration)
{
    SolRayResult result = Sol_Raycast(world, ray);
    Sol_Line_Add(world, (LineDesc){
                            .a      = ray.pos,
                            .b      = result.pos,
                            .colorA = (vec4s){1, 0, 0, 1},
                            .colorB = (vec4s){1, 0, 0, 1},
                            .ttl    = debugDuration,
                        });
    if (result.hit)
        Sol_Line_Add(world, (LineDesc){
                                .a      = result.pos,
                                .b      = glms_vec3_add(result.pos, glms_vec3_scale(ray.dir, ray.dist - result.dist)),
                                .colorA = (vec4s){0, 1, 0, 1},
                                .colorB = (vec4s){0, 1, 0, 1},
                                .ttl    = debugDuration,
                            });
    return result;
}

SolRayResult Raycast_Static_Grid_Tri(PhysxGroup *group, SolRay ray)
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

SolRayResult Raycast_Dynamic_Table_Ent(PhysxGroup *group, SolRay ray, World *world)
{
    SolRayResult result   = {0};
    vec3s        origin   = ray.pos;
    vec3s        dir      = ray.dir;
    float        dist     = ray.dist;
    int          cellWalk = 0;
    while (1)
    {
        vec3s checkPos = glms_vec3_add(origin, glms_vec3_scale(dir, group->table.cellSize * cellWalk));
        cellWalk++;
        int         checks = 0;
        SpatialCell cell   = Spatial_Cell_Get(checkPos, group->table.cellSize);
        for (int c = 0; c < 27; c++)
        {
            u32 entry = group->table.head[cell.neighborHashes[c] & (group->table.size - 1)];
            while (entry != SPATIAL_NULL)
            {
                if (checks > 0x1ff)
                    break;
                u32 id = group->table.value[entry];

                entry = group->table.next[entry];
            }
        }
    }

    return result;
}

void Grid_Walker_Init(GridWalker *w, SolRay ray, SpatialGrid *grid)
{
    if (!grid->offsets)
        return;
    float cellSize = grid->cellSize;
    vec3s gridMin  = grid->min;
    vec3s pos      = ray.pos;
    vec3s dir      = ray.dir;

    w->ix = (int)floorf((pos.x - gridMin.x) / cellSize);
    w->iy = (int)floorf((pos.y - gridMin.y) / cellSize);
    w->iz = (int)floorf((pos.z - gridMin.z) / cellSize);

    w->dims    = grid->dims;
    w->bounded = true;
    w->maxDist = ray.dist;
    w->tEntry  = 0.0f;

    w->stepX = dir.x > 0 ? 1 : -1;
    w->stepY = dir.y > 0 ? 1 : -1;
    w->stepZ = dir.z > 0 ? 1 : -1;

    float nextX = gridMin.x + (w->ix + (w->stepX > 0 ? 1 : 0)) * cellSize;
    float nextY = gridMin.y + (w->iy + (w->stepY > 0 ? 1 : 0)) * cellSize;
    float nextZ = gridMin.z + (w->iz + (w->stepZ > 0 ? 1 : 0)) * cellSize;

    w->tMaxX = (fabsf(dir.x) > 1e-8f) ? (nextX - pos.x) / dir.x : INFINITY;
    w->tMaxY = (fabsf(dir.y) > 1e-8f) ? (nextY - pos.y) / dir.y : INFINITY;
    w->tMaxZ = (fabsf(dir.z) > 1e-8f) ? (nextZ - pos.z) / dir.z : INFINITY;

    w->tDeltaX = (fabsf(dir.x) > 1e-8f) ? cellSize / fabsf(dir.x) : INFINITY;
    w->tDeltaY = (fabsf(dir.y) > 1e-8f) ? cellSize / fabsf(dir.y) : INFINITY;
    w->tDeltaZ = (fabsf(dir.z) > 1e-8f) ? cellSize / fabsf(dir.z) : INFINITY;
}

void Grid_Walker_Init_Infinite(GridWalker *w, SolRay ray, float cellSize)
{
    float maxDist = ray.dist;
    vec3s dir     = ray.dir;
    vec3s pos     = ray.pos;

    w->ix = (int)floorf(pos.x / cellSize);
    w->iy = (int)floorf(pos.y / cellSize);
    w->iz = (int)floorf(pos.z / cellSize);

    w->bounded = false;
    w->maxDist = maxDist;
    w->tEntry  = 0.0f;

    w->stepX = dir.x > 0 ? 1 : -1;
    w->stepY = dir.y > 0 ? 1 : -1;
    w->stepZ = dir.z > 0 ? 1 : -1;

    float nextX = (w->ix + (w->stepX > 0 ? 1 : 0)) * cellSize;
    float nextY = (w->iy + (w->stepY > 0 ? 1 : 0)) * cellSize;
    float nextZ = (w->iz + (w->stepZ > 0 ? 1 : 0)) * cellSize;

    w->tMaxX = (fabsf(dir.x) > 1e-8f) ? (nextX - pos.x) / dir.x : INFINITY;
    w->tMaxY = (fabsf(dir.y) > 1e-8f) ? (nextY - pos.y) / dir.y : INFINITY;
    w->tMaxZ = (fabsf(dir.z) > 1e-8f) ? (nextZ - pos.z) / dir.z : INFINITY;

    w->tDeltaX = (fabsf(dir.x) > 1e-8f) ? cellSize / fabsf(dir.x) : INFINITY;
    w->tDeltaY = (fabsf(dir.y) > 1e-8f) ? cellSize / fabsf(dir.y) : INFINITY;
    w->tDeltaZ = (fabsf(dir.z) > 1e-8f) ? cellSize / fabsf(dir.z) : INFINITY;
}

bool Grid_Walker_Next(GridWalker *w, GridCell *out)
{
    // Stop if ray has reached max distance
    if (w->tEntry >= w->maxDist)
        return false;

    // Stop if current cell is outside bounded grid
    if (w->bounded &&
        (w->ix < 0 || w->ix >= w->dims.x || w->iy < 0 || w->iy >= w->dims.y || w->iz < 0 || w->iz >= w->dims.z))
        return false;

    // Fill current cell
    out->ix     = w->ix;
    out->iy     = w->iy;
    out->iz     = w->iz;
    out->tEntry = w->tEntry;
    out->tExit  = fminf(w->tMaxX, fminf(w->tMaxY, w->tMaxZ));
    if (out->tExit > w->maxDist)
        out->tExit = w->maxDist;

    // Advance state for next call
    if (w->tMaxX < w->tMaxY && w->tMaxX < w->tMaxZ)
    {
        w->tEntry = w->tMaxX;
        w->ix += w->stepX;
        w->tMaxX += w->tDeltaX;
    }
    else if (w->tMaxY < w->tMaxZ)
    {
        w->tEntry = w->tMaxY;
        w->iy += w->stepY;
        w->tMaxY += w->tDeltaY;
    }
    else
    {
        w->tEntry = w->tMaxZ;
        w->iz += w->stepZ;
        w->tMaxZ += w->tDeltaZ;
    }

    return true;
}

SolRayResult Raycast_Static_Grid_Walk(World *world, SolRay ray)
{
    SolRayResult result = {0};
    result.dist         = ray.dist;
    result.pos          = vecAdd(ray.pos, vecSca(ray.dir, ray.dist));
    PhysxGroup  *group  = &world->spatial->staticGroup;
    SpatialGrid *grid   = &world->spatial->staticGroup.grid;

    if (!grid->offsets)
        return result;

    GridWalker walker;
    Grid_Walker_Init(&walker, ray, grid);

    GridCell cell;
    while (Grid_Walker_Next(&walker, &cell))
    {
        u32 cellIdx = cell.ix + cell.iy * grid->dims.x + cell.iz * grid->dims.x * grid->dims.y;
        u32 start   = grid->offsets[cellIdx];
        u32 end     = grid->offsets[cellIdx + 1];

        for (u32 e = start; e < end; e++)
        {
            SolTri *tri = &group->tris[grid->values[e]];
            vec3s   normal;
            float   t = Ray_Tri_Test(ray.pos, ray.dir, tri, &normal);

            if (t > 0 && t <= result.dist)
            {
                result.dist     = t;
                result.hit      = true;
                result.norm     = normal;
                result.triIndex = grid->values[e];
                result.pos      = glms_vec3_add(ray.pos, glms_vec3_scale(ray.dir, t));
                result.entId    = tri->entId;
            }
        }

        // Early out: if we have a hit within this cell's span, no closer hit possible
        if (result.hit && result.dist <= cell.tExit)
            break;
    }

    return result;
}

SolRayResult Raycast_Dynamic_Table_Walk(World *world, SolRay ray)
{
    SolRayResult result = {0};
    result.dist         = ray.dist;

    PhysxGroup   *group = &world->spatial->dynamicGroup;
    SpatialTable *table = &group->table;

    GridWalker walker;
    Grid_Walker_Init_Infinite(&walker, ray, table->cellSize);

    GridCell cell;
    while (Grid_Walker_Next(&walker, &cell))
    {
        u32 hash  = hash_coords(cell.ix, cell.iy, cell.iz) & (table->size - 1);
        u32 entry = table->head[hash];

        while (entry != SPATIAL_NULL)
        {
            u32 id = table->value[entry];
            if (id != ray.ignoreEnt)
            {
                CompBody *body = &world->bodies[id];
                if (ray.mask & body->group)
                {
                    vec3s normal;
                    float t = -1.0f;

                    RaycastTest resolver = ray_shape_test[body->shape];
                    if (resolver)
                        t = resolver(ray, &world->xforms[id], body, &normal);

                    if (t > 0 && t <= result.dist)
                    {
                        result.dist  = t;
                        result.hit   = true;
                        result.norm  = normal;
                        result.entId = id;
                        result.pos   = glms_vec3_add(ray.pos, glms_vec3_scale(ray.dir, t));
                    }
                }
            }

            entry = table->next[entry];
        }
        if (result.hit && result.dist <= cell.tExit)
            break;
    }

    return result;
}