/*
 * File: s_body3.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-28
 *
 */
#include "s_physx.h"
#include "sol/types.h"
#include "world.h"
#include "model.h"
#include "sol_math.h"
#include "profiler.h"
#include "sol_user.h"
#include "sol_core.h"

#include <omp.h>

static int user_contact_count = 0;

static SolProfiler prof2  = {.name = "Dynamic"};
static SolProfiler prof3  = {.name = "Static"};
const static float sub_dt = (float)SOL_TIMESTEP * (1.0f / (float)SOLVER_ITERATIONS);

void Physx_Step(World *world, double dt)
{
    float     fdt = (float)dt;
    int       i, j, k, l, m, iter;
    SysPhysx *ws = world->systems[WORLDSYS_PHYSX];

    SparseSet_ScBody3 *set   = Sol_Comp_Set(world, ScBody3);
    int                count = set->cnt;
    for (i = 0; i < set->cnt; i++)
    {
        int      id    = set->dense[i];
        ScBody3 *body3 = &set->data[i];
        ScXform *xform = Sol_Comp_Get(world, id, ScXform);
        if (!xform || body3->mass == 0.0f)
            continue;

        body3->vel     = glms_vec3_scale(body3->vel, 0.99f);
        vec3s accel    = body3->vel.y < TERMINAL_VELOCITY ? GLMS_VEC3_ZERO : body3->gravity;
        accel          = glms_vec3_add(accel, body3->force);
        accel          = glms_vec3_add(accel, body3->impulse);
        body3->impulse = (vec3s){0};
        body3->vel     = glms_vec3_add(body3->vel, glms_vec3_scale(accel, fdt));
    }

    Build_Tables(world, ws, fdt);
    for (iter = 0; iter < SOLVER_ITERATIONS; iter++)
    {
        for (i = 0; i < set->cnt; i++)
        {
            int      id    = set->dense[i];
            ScBody3 *body  = &set->data[i];
            ScXform *xform = Sol_Comp_Get(world, id, ScXform);
            xform->pos     = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, fdt / (float)SOLVER_ITERATIONS));
        }
        solb_zero(ws->contacts);
#pragma omp parallel if (count > 100)
        {
            ThreadContactBuffer local_buf = {0};
            if (ws->dynamic_group.spatial.item_count > 0)
            {
#pragma omp for schedule(dynamic)
                for (j = 0; j < set->cnt; j++)
                {
                    int      id    = set->dense[j];
                    ScBody3 *body  = &set->data[j];
                    ScXform *xform = Sol_Comp_Get(world, id, ScXform);
                    if (!xform || body->mass == 0.0f)
                        continue;

                    vec3s min = vecSub(xform->pos, body->dims);
                    vec3s max = vecAdd(xform->pos, body->dims);
                    Collisions_Dynamic_Bodies_Local(world, id, body->shape, min, max, &ws->dynamic_group, &local_buf);
                }
            }
            if (ws->static_group.spatial.item_count > 0)
            {
#pragma omp for schedule(dynamic)
                for (k = 0; k < set->cnt; k++)
                {
                    int      id    = set->dense[k];
                    ScBody3 *body  = &set->data[k];
                    ScXform *xform = Sol_Comp_Get(world, id, ScXform);
                    if (!xform || body->mass == 0.0f)
                        continue;

                    vec3s min = vecSub(xform->pos, body->dims);
                    vec3s max = vecAdd(xform->pos, body->dims);
                    Collisions_Static_Stage_Local(world, id, body->shape, min, max, &ws->static_group, &local_buf);
                }
            }
            if (local_buf.count > 0)
            {
#pragma omp critical
                {
                    solb_push_array(ws->contacts, local_buf.contacts, local_buf.count);
                }
            }
        }
#pragma omp for schedule(dynamic)
        for (l = 0; l < solb_count(ws->contacts); l++)
        {
            SolContact *contact = &ws->contacts[l];

            ScBody3 *bodyA  = Sol_Comp_Get(world, contact->id, ScBody3);
            ScXform *xformA = Sol_Comp_Get(world, contact->id, ScXform);
            ScBody3 *bodyB  = Sol_Comp_Get(world, contact->idB, ScBody3);
            ScXform *xformB = Sol_Comp_Get(world, contact->idB, ScXform);

            Resolve_Contact(bodyA, xformA, bodyB, xformB, contact);
        }
    }

    // -------------------------------------------------------------
    // 5. Out-of-bounds reset loop DEBUG
    // -------------------------------------------------------------
    for (m = 0; m < set->cnt; m++)
    {
        int      id    = set->dense[m];
        ScXform *xform = Sol_Comp_Get(world, id, ScXform);
        if (xform && xform->pos.y < -20.0f)
        {
            Sol_Xform_Teleport(world, id, (vec3s){0, 50, 0});
        }
    }
}

void Physx_Init(World *world)
{
    SysPhysx *ws                   = malloc(sizeof(SysPhysx));
    world->systems[WORLDSYS_PHYSX] = ws;
    SpatialGrid_Init(&ws->dynamic_group.spatial, DYNAMIC_CELL_SIZE);
    SpatialGrid_Init(&ws->static_group.spatial, STATIC_CELL_SIZE);
    solb_init(ws->dynamic_group.aabb_scratch, 16);
    solb_init(ws->contacts, MAX_CONTACTS);
    solb_init(ws->static_group.tris, WORLD_TRI_INIT);
}

void Physx_Deinit(World *world)
{
    SysPhysx *ws = world->systems[WORLDSYS_PHYSX];
    if (ws)
    {
        SpatialGrid_Destroy(&ws->dynamic_group.spatial);
        SpatialGrid_Destroy(&ws->static_group.spatial);
        free(ws);
        world->systems[WORLDSYS_PHYSX] = NULL;
    }
}

bool Sol_Physx_DoesCollide(ScBody3 *body, ScBody3 *other_body)
{
    return (body->mask << 16) & (other_body->mask);
}

vec3s Sol_Physx_GetGround(World *world, int id)
{
    return GLMS_VEC3_ZERO;
}

int Sol_Physx_RaycastD(World *world, SolRay ray, SolRayResult *result, int max, float time)
{
    int hits = Sol_Physx_Raycast(world, ray, result, max);

    SolLine *line = Sol_Line_New(world);
    line->a       = ray.start;
    vec3s end     = vecAdd(ray.start, vecSca(ray.dir, ray.dist));
    if (result[0].hit)
    {
        line->b        = result[0].pos;
        SolLine *line2 = Sol_Line_New(world);
        line2->a       = result[0].pos;
        line2->b       = end;
        line2->aColor  = VEC4_GREEN;
        line2->bColor  = VEC4_GREEN;
        line2->ttl     = time;
    }
    else
        line->b = end;
    line->aColor = VEC4_RED;
    line->bColor = VEC4_RED;
    line->ttl    = time;
    return hits;
}

bool Sol_Physx_RaycastFirstD(World *world, SolRay ray, SolRayResult *result, float time)
{
    bool hit = Sol_Physx_RaycastFirst(world, ray, result);

    SolLine *line = Sol_Line_New(world);
    line->a       = ray.start;
    if (result->hit)
        line->b = result->pos;
    else
        line->b = vecAdd(ray.start, vecSca(ray.dir, ray.dist));
    line->aColor = VEC4_RED;
    line->bColor = VEC4_RED;
    line->ttl    = time;

    return hit;
}

bool Sol_Physx_RaycastFirst(World *world, SolRay ray, SolRayResult *outResult)
{
    if (!outResult || ray.dist <= 0.0f)
        return false;

    SysPhysx *ws = world->systems[WORLDSYS_PHYSX];
    if (!ws)
        return false;

    bool  hitFound = false;
    float maxDist  = ray.dist; // Clamped dynamically as closer hits are found

    SpatialGrid *grids[2]    = {&ws->static_group.spatial, &ws->dynamic_group.spatial};
    bool         isStatic[2] = {true, false};

    for (int g = 0; g < 2; g++)
    {
        SpatialGrid *grid = grids[g];
        if (grid->item_count == 0)
            continue;

        ivec3s cell  = SpatialGrid_WorldToCell(grid, ray.start);
        int    stepX = (ray.dir.x > 0) ? 1 : ((ray.dir.x < 0) ? -1 : 0);
        int    stepY = (ray.dir.y > 0) ? 1 : ((ray.dir.y < 0) ? -1 : 0);
        int    stepZ = (ray.dir.z > 0) ? 1 : ((ray.dir.z < 0) ? -1 : 0);

        vec3s cellSize = {(grid->max.x - grid->min.x) / (float)grid->dims.x,
                          (grid->max.y - grid->min.y) / (float)grid->dims.y,
                          (grid->max.z - grid->min.z) / (float)grid->dims.z};

        float tDeltaX = (stepX != 0) ? fabsf(cellSize.x / ray.dir.x) : 1e30f;
        float tDeltaY = (stepY != 0) ? fabsf(cellSize.y / ray.dir.y) : 1e30f;
        float tDeltaZ = (stepZ != 0) ? fabsf(cellSize.z / ray.dir.z) : 1e30f;

        vec3s cellMin = {grid->min.x + cell.x * cellSize.x, grid->min.y + cell.y * cellSize.y,
                         grid->min.z + cell.z * cellSize.z};

        float tMaxX = (stepX > 0)   ? (cellMin.x + cellSize.x - ray.start.x) / ray.dir.x
                      : (stepX < 0) ? (cellMin.x - ray.start.x) / ray.dir.x
                                    : 1e30f;
        float tMaxY = (stepY > 0)   ? (cellMin.y + cellSize.y - ray.start.y) / ray.dir.y
                      : (stepY < 0) ? (cellMin.y - ray.start.y) / ray.dir.y
                                    : 1e30f;
        float tMaxZ = (stepZ > 0)   ? (cellMin.z + cellSize.z - ray.start.z) / ray.dir.z
                      : (stepZ < 0) ? (cellMin.z - ray.start.z) / ray.dir.z
                                    : 1e30f;

        float currentCellT = 0.0f;

        // Terminate immediately when cell distance exceeds our current closest hit
        while (cell.x >= 0 && cell.x < grid->dims.x && cell.y >= 0 && cell.y < grid->dims.y && cell.z >= 0 &&
               cell.z < grid->dims.z && currentCellT < maxDist)
        {
            uint32_t cellIdx = SpatialGrid_GetCellIndex(grid, cell.x, cell.y, cell.z);
            GridCell gCell   = grid->cells[cellIdx];

            for (uint32_t i = 0; i < gCell.count; i++)
            {
                uint32_t idx = grid->index_buffer[gCell.offset + i];

                if (isStatic[g])
                {
                    const SolTri *tri = &ws->static_group.tris[idx];

                    float tHit;
                    vec3s normHit;
                    if (Ray_Intersect_Tri(ray.start, ray.dir, maxDist, tri, &tHit, &normHit))
                    {
                        if (tHit < maxDist)
                        {
                            maxDist          = tHit; // Tighten ray search bound
                            outResult->hit   = true;
                            outResult->dist  = tHit;
                            outResult->pos   = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, tHit));
                            outResult->norm  = normHit;
                            outResult->entId = tri->entId;
                            hitFound         = true;
                        }
                    }
                }
                else
                {
                    int entId = (int)idx;

                    ScBody3 *body  = Sol_Comp_Get(world, entId, ScBody3);
                    ScXform *xform = Sol_Comp_Get(world, entId, ScXform);

                    if (!body || !xform || !(body->mask & ray.mask))
                        continue;

                    float tHit;
                    vec3s normHit;
                    if (body->shape == SHAPE3_SPH)
                    {
                        if (Ray_Intersect_Sphere(ray.start, ray.dir, maxDist, xform->pos, body->dims.x, &tHit,
                                                 &normHit))
                        {
                            if (tHit < maxDist)
                            {
                                maxDist          = tHit; // Tighten ray search bound
                                outResult->hit   = true;
                                outResult->dist  = tHit;
                                outResult->pos   = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, tHit));
                                outResult->norm  = normHit;
                                outResult->entId = entId;
                                hitFound         = true;
                            }
                        }
                    }
                }
            }

            // Step DDA to adjacent cell
            if (tMaxX < tMaxY)
            {
                if (tMaxX < tMaxZ)
                {
                    currentCellT = tMaxX;
                    cell.x += stepX;
                    tMaxX += tDeltaX;
                }
                else
                {
                    currentCellT = tMaxZ;
                    cell.z += stepZ;
                    tMaxZ += tDeltaZ;
                }
            }
            else
            {
                if (tMaxY < tMaxZ)
                {
                    currentCellT = tMaxY;
                    cell.y += stepY;
                    tMaxY += tDeltaY;
                }
                else
                {
                    currentCellT = tMaxZ;
                    cell.z += stepZ;
                    tMaxZ += tDeltaZ;
                }
            }
        }
    }

    return hitFound;
}

int Sol_Physx_Raycast(World *world, SolRay ray, SolRayResult *result, int max)
{
    if (!result || max <= 0 || ray.dist <= 0.0f)
        return 0;

    SysPhysx *ws   = world->systems[WORLDSYS_PHYSX];
    int       hits = 0;

    vec3s rayEnd = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, ray.dist));

    // We step through static and dynamic spatial grids
    SpatialGrid *grids[2]    = {&ws->static_group.spatial, &ws->dynamic_group.spatial};
    bool         isStatic[2] = {true, false};

    for (int g = 0; g < 2; g++)
    {
        SpatialGrid *grid = grids[g];
        if (grid->item_count == 0)
            continue;

        ivec3s cell  = SpatialGrid_WorldToCell(grid, ray.start);
        int    stepX = (ray.dir.x > 0) ? 1 : ((ray.dir.x < 0) ? -1 : 0);
        int    stepY = (ray.dir.y > 0) ? 1 : ((ray.dir.y < 0) ? -1 : 0);
        int    stepZ = (ray.dir.z > 0) ? 1 : ((ray.dir.z < 0) ? -1 : 0);

        vec3s cellSize = {(grid->max.x - grid->min.x) / (float)grid->dims.x,
                          (grid->max.y - grid->min.y) / (float)grid->dims.y,
                          (grid->max.z - grid->min.z) / (float)grid->dims.z};

        float tDeltaX = (stepX != 0) ? fabsf(cellSize.x / ray.dir.x) : 1e30f;
        float tDeltaY = (stepY != 0) ? fabsf(cellSize.y / ray.dir.y) : 1e30f;
        float tDeltaZ = (stepZ != 0) ? fabsf(cellSize.z / ray.dir.z) : 1e30f;

        vec3s cellMin = {grid->min.x + cell.x * cellSize.x, grid->min.y + cell.y * cellSize.y,
                         grid->min.z + cell.z * cellSize.z};

        float tMaxX = (stepX > 0)   ? (cellMin.x + cellSize.x - ray.start.x) / ray.dir.x
                      : (stepX < 0) ? (cellMin.x - ray.start.x) / ray.dir.x
                                    : 1e30f;
        float tMaxY = (stepY > 0)   ? (cellMin.y + cellSize.y - ray.start.y) / ray.dir.y
                      : (stepY < 0) ? (cellMin.y - ray.start.y) / ray.dir.y
                                    : 1e30f;
        float tMaxZ = (stepZ > 0)   ? (cellMin.z + cellSize.z - ray.start.z) / ray.dir.z
                      : (stepZ < 0) ? (cellMin.z - ray.start.z) / ray.dir.z
                                    : 1e30f;

        float currentCellT = 0.0f;

        while (cell.x >= 0 && cell.x < grid->dims.x && cell.y >= 0 && cell.y < grid->dims.y && cell.z >= 0 &&
               cell.z < grid->dims.z && currentCellT < ray.dist && hits < max)
        {
            uint32_t cellIdx = SpatialGrid_GetCellIndex(grid, cell.x, cell.y, cell.z);
            GridCell gCell   = grid->cells[cellIdx];

            for (uint32_t i = 0; i < gCell.count && hits < max; i++)
            {
                uint32_t idx = grid->index_buffer[gCell.offset + i];

                if (isStatic[g])
                {
                    const SolTri *tri   = &ws->static_group.tris[idx];
                    int           entId = tri->entId;

                    if (Is_Already_Hit(result, hits, entId))
                        continue;

                    float tHit;
                    vec3s normHit;
                    if (Ray_Intersect_Tri(ray.start, ray.dir, ray.dist, tri, &tHit, &normHit))
                    {
                        result[hits].hit   = true;
                        result[hits].dist  = tHit;
                        result[hits].pos   = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, tHit));
                        result[hits].norm  = normHit;
                        result[hits].entId = entId;
                        hits++;
                    }
                }
                else
                {
                    int entId = (int)idx;

                    if (Is_Already_Hit(result, hits, entId))
                        continue;

                    ScBody3 *body  = Sol_Comp_Get(world, entId, ScBody3);
                    ScXform *xform = Sol_Comp_Get(world, entId, ScXform);

                    if (!body || !xform || !(body->mask & ray.mask))
                        continue;

                    float tHit;
                    vec3s normHit;
                    if (body->shape == SHAPE3_SPH)
                    {
                        if (Ray_Intersect_Sphere(ray.start, ray.dir, ray.dist, xform->pos, body->dims.x, &tHit,
                                                 &normHit))
                        {
                            result[hits].hit   = true;
                            result[hits].dist  = tHit;
                            result[hits].pos   = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, tHit));
                            result[hits].norm  = normHit;
                            result[hits].entId = entId;
                            hits++;
                        }
                    }
                }
            }

            // Step DDA to adjacent cell
            if (tMaxX < tMaxY)
            {
                if (tMaxX < tMaxZ)
                {
                    currentCellT = tMaxX;
                    cell.x += stepX;
                    tMaxX += tDeltaX;
                }
                else
                {
                    currentCellT = tMaxZ;
                    cell.z += stepZ;
                    tMaxZ += tDeltaZ;
                }
            }
            else
            {
                if (tMaxY < tMaxZ)
                {
                    currentCellT = tMaxY;
                    cell.y += stepY;
                    tMaxY += tDeltaY;
                }
                else
                {
                    currentCellT = tMaxZ;
                    cell.z += stepZ;
                    tMaxZ += tDeltaZ;
                }
            }
        }
    }

    return hits;
}

int Sol_Physx_Spherecast(World *world, SolRay ray, float radius, SolRayResult *result, int max)
{
    if (!result || max <= 0 || ray.dist <= 0.0f)
        return 0;

    SysPhysx *ws   = world->systems[WORLDSYS_PHYSX];
    int       hits = 0;

    // Compute bounding box encompassing the entire swept capsule path
    vec3s rayEnd = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, ray.dist));
    vec3s minP   = glms_vec3_sub(glms_vec3_minv(ray.start, rayEnd), (vec3s){radius, radius, radius});
    vec3s maxP   = glms_vec3_add(glms_vec3_maxv(ray.start, rayEnd), (vec3s){radius, radius, radius});

    SpatialGrid *grids[2]    = {&ws->static_group.spatial, &ws->dynamic_group.spatial};
    bool         isStatic[2] = {true, false};

    for (int g = 0; g < 2; g++)
    {
        SpatialGrid *grid = grids[g];
        if (grid->item_count == 0)
            continue;

        ivec3s minCell = SpatialGrid_WorldToCell(grid, minP);
        ivec3s maxCell = SpatialGrid_WorldToCell(grid, maxP);

        minCell.x = clampi(minCell.x, 0, grid->dims.x - 1);
        maxCell.x = clampi(maxCell.x, 0, grid->dims.x - 1);
        minCell.y = clampi(minCell.y, 0, grid->dims.y - 1);
        maxCell.y = clampi(maxCell.y, 0, grid->dims.y - 1);
        minCell.z = clampi(minCell.z, 0, grid->dims.z - 1);
        maxCell.z = clampi(maxCell.z, 0, grid->dims.z - 1);

        for (int z = minCell.z; z <= maxCell.z && hits < max; z++)
        {
            for (int y = minCell.y; y <= maxCell.y && hits < max; y++)
            {
                for (int x = minCell.x; x <= maxCell.x && hits < max; x++)
                {
                    uint32_t cellIdx = SpatialGrid_GetCellIndex(grid, x, y, z);
                    GridCell gCell   = grid->cells[cellIdx];

                    for (uint32_t i = 0; i < gCell.count && hits < max; i++)
                    {
                        uint32_t idx = grid->index_buffer[gCell.offset + i];

                        if (isStatic[g])
                        {
                            const SolTri *tri   = &ws->static_group.tris[idx];
                            int           entId = tri->entId;

                            if (Is_Already_Hit(result, hits, entId))
                                continue;

                            // Spherecast vs Static Triangle (Approximated via ray vs expanded face plane)
                            float tHit;
                            vec3s normHit;
                            if (Ray_Intersect_Tri(ray.start, ray.dir, ray.dist, tri, &tHit, &normHit))
                            {
                                result[hits].hit   = true;
                                result[hits].dist  = tHit;
                                result[hits].pos   = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, tHit));
                                result[hits].norm  = normHit;
                                result[hits].entId = entId;
                                hits++;
                            }
                        }
                        else
                        {
                            int entId = (int)idx;

                            if (Is_Already_Hit(result, hits, entId))
                                continue;

                            ScBody3 *body  = Sol_Comp_Get(world, entId, ScBody3);
                            ScXform *xform = Sol_Comp_Get(world, entId, ScXform);

                            if (!body || !xform || !(body->mask & ray.mask))
                                continue;

                            float tHit;
                            vec3s normHit;

                            // Dynamic Spherecast: Ray vs Minkowski Expanded Sphere (R_body + R_cast)
                            if (body->shape == SHAPE3_SPH)
                            {
                                float expandedRadius = body->dims.x + radius;
                                if (Ray_Intersect_Sphere(ray.start, ray.dir, ray.dist, xform->pos, expandedRadius,
                                                         &tHit, &normHit))
                                {
                                    result[hits].hit   = true;
                                    result[hits].dist  = tHit;
                                    result[hits].pos   = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, tHit));
                                    result[hits].norm  = normHit; // Points from sphere center outward
                                    result[hits].entId = entId;
                                    hits++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return hits;
}