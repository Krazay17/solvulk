/*
 * File: s_body3.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-28
 *
 */
#include "s_body3.h"
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

void Body3_Step(World *world, double dt)
{
    float     fdt = (float)dt;
    int       i, j, k, l, m, iter;
    SysPhysx *ws = world->systems[WORLDSYS_BODY3];

    SparseSet_ScBody3 *set   = Sol_Comp_Set(world, ScBody3);
    int                count = set->cnt;
    for (i = 0; i < set->cnt; i++)
    {
        int      id    = set->dense[i];
        ScBody3 *body3 = &set->data[i];
        ScXform *xform = Sol_Comp_Get(world, id, ScXform);
        if (!xform || body3->mass == 0.0f)
            continue;

        body3->vel     = glms_vec3_scale(body3->vel, 0.999f);
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
    world->systems[WORLDSYS_BODY3] = ws;
    SpatialGrid_Init(&ws->dynamic_group.spatial, DYNAMIC_CELL_SIZE);
    SpatialGrid_Init(&ws->static_group.spatial, STATIC_CELL_SIZE);
    solb_init(ws->dynamic_group.aabb_scratch, 16);
    solb_init(ws->contacts, MAX_CONTACTS);
    solb_init(ws->static_group.tris, WORLD_TRI_INIT);
}

void Physx_Deinit(World *world)
{
    SysPhysx *ws = world->systems[WORLDSYS_BODY3];
    if (ws)
    {
        SpatialGrid_Destroy(&ws->dynamic_group.spatial);
        SpatialGrid_Destroy(&ws->static_group.spatial);
        free(ws);
        world->systems[WORLDSYS_BODY3] = NULL;
    }
}

bool Sol_Body3_DoesCollide(ScBody3 *body, ScBody3 *other_body)
{
    return (body->mask << 16) & (other_body->mask);
}

vec3s Sol_Body3_GetGround(World *world, int id)
{
    return GLMS_VEC3_ZERO;
}

int Sol_RaycastD(World *world, SolRay ray, SolRayResult *result, int max, float time)
{
    int hits = Sol_Raycast(world, ray, result, max);

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

bool Sol_Raycast1D(World *world, SolRay ray, SolRayResult *result, float time)
{
    bool hit = Sol_Raycast1(world, ray, result);

    SolLine *line = Sol_Line_New(world);
    line->a       = ray.start;
    if (hit && result)
        line->b = result->pos;
    else
        line->b = vecAdd(ray.start, vecSca(ray.dir, ray.dist));
    line->aColor = VEC4_RED;
    line->bColor = VEC4_RED;
    line->ttl    = time;

    return hit;
}

bool Sol_Raycast1(World *world, SolRay ray, SolRayResult *outResult)
{
    bool debugThisRay = ray.debug;
    if (ray.dist <= 0.0f)
        return false;
    if (glms_vec3_dot(ray.dir, ray.dir) < 1e-12f)
        return false; // degenerate direction
    ray.dir = glms_vec3_normalize(ray.dir);

    SysPhysx *ws = world->systems[WORLDSYS_BODY3];
    if (!ws)
        return false;

    bool  hitFound = false;
    float maxDist  = ray.dist;

    SpatialGrid *grids[2]    = {&ws->static_group.spatial, &ws->dynamic_group.spatial};
    bool         isStatic[2] = {true, false};

    for (int g = 0; g < 2; g++)
    {
        SpatialGrid *grid = grids[g];
        if (grid->item_count == 0)
            continue;

        GridDDA it           = GridDDA_Init(grid, ray.start, ray.dir);
        float   currentCellT = 0.0f;

        while (GridDDA_InBounds(&it, grid) && currentCellT < maxDist)
        {
            uint32_t cellIdx = SpatialGrid_GetCellIndexUnchecked(grid, it.cell.x, it.cell.y, it.cell.z);
            GridCell gCell   = grid->cells[cellIdx];

            for (uint32_t i = 0; i < gCell.count; i++)
            {
                uint32_t idx = grid->index_buffer[gCell.offset + i];

                if (isStatic[g])
                {
                    const SolTri *tri = &ws->static_group.tris[idx];
                    if (tri->entId == ray.ignoreEnt)
                        continue;

                    float tHit; vec3s normHit;
                    if (Ray_Intersect_Tri(ray.start, ray.dir, maxDist, tri, &tHit, &normHit))
                    {
                        if (debugThisRay)
                            printf("HIT tHit=%.4f entId=%d\n", tHit, tri->entId);
                        if (!outResult)
                            return true;

                        hitFound = true;
                        maxDist  = tHit; // Ray_Intersect_Tri already enforced t <= maxDist -- no recheck needed

                        outResult->hit   = true;
                        outResult->dist  = tHit;
                        outResult->pos   = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, tHit));
                        outResult->norm  = normHit;
                        outResult->entId = tri->entId;
                    }
                }
                else
                {
                    int entId = (int)idx;
                    if (entId == ray.ignoreEnt)
                        continue;

                    ScBody3 *body  = Sol_Comp_Get(world, entId, ScBody3);
                    ScXform *xform = Sol_Comp_Get(world, entId, ScXform);
                    if (!body || !xform || !(body->mask & ray.mask))
                        continue;

                    if (body->shape == SHAPE3_SPH)
                    {
                        float tHit; vec3s normHit;
                        if (Ray_Intersect_Sphere(ray.start, ray.dir, maxDist, xform->pos, body->dims.x, &tHit,
                                                 &normHit))
                        {
                            if (!outResult)
                                return true;

                            hitFound = true;
                            maxDist  = tHit;

                            outResult->hit   = true;
                            outResult->dist  = tHit;
                            outResult->pos   = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, tHit));
                            outResult->norm  = normHit;
                            outResult->entId = entId;
                        }
                    }
                }
            }

            currentCellT = GridDDA_Step(&it);
        }
    }

    return hitFound;
}

int Sol_Raycast(World *world, SolRay ray, SolRayResult *result, int max)
{
    if (!result || max <= 0 || ray.dist <= 0.0f)
        return 0;
    if (glms_vec3_dot(ray.dir, ray.dir) < 1e-12f)
        return 0;
    ray.dir = glms_vec3_normalize(ray.dir);

    SysPhysx *ws   = world->systems[WORLDSYS_BODY3];
    int       hits = 0;

    SpatialGrid *grids[2]    = {&ws->static_group.spatial, &ws->dynamic_group.spatial};
    bool         isStatic[2] = {true, false};

    for (int g = 0; g < 2 && hits < max; g++)
    {
        SpatialGrid *grid = grids[g];
        if (grid->item_count == 0)
            continue;

        GridDDA it           = GridDDA_Init(grid, ray.start, ray.dir); // now uses grid->cellSize -- bug fixed here too
        float   currentCellT = 0.0f;

        while (GridDDA_InBounds(&it, grid) && currentCellT < ray.dist && hits < max)
        {
            uint32_t cellIdx = SpatialGrid_GetCellIndexUnchecked(grid, it.cell.x, it.cell.y, it.cell.z);
            GridCell gCell   = grid->cells[cellIdx];

            for (uint32_t i = 0; i < gCell.count && hits < max; i++)
            {
                uint32_t idx = grid->index_buffer[gCell.offset + i];

                if (isStatic[g])
                {
                    const SolTri *tri   = &ws->static_group.tris[idx];
                    int           entId = tri->entId;
                    // NOTE: per-entity dedup -- see write-up above. A single large
                    // static mesh entity will only ever report one hit through this path.
                    if (entId == ray.ignoreEnt || Is_Already_Hit(result, hits, entId))
                        continue;

                    float tHit; vec3s normHit;
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
                    if (entId == ray.ignoreEnt || Is_Already_Hit(result, hits, entId))
                        continue;

                    ScBody3 *body  = Sol_Comp_Get(world, entId, ScBody3);
                    ScXform *xform = Sol_Comp_Get(world, entId, ScXform);
                    if (!body || !xform || !(body->mask & ray.mask))
                        continue;

                    if (body->shape == SHAPE3_SPH)
                    {
                        float tHit; vec3s normHit;
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

            currentCellT = GridDDA_Step(&it);
        }
    }

    // Multi-hit results were never guaranteed sorted -- Sol_RaycastD's debug
    // line (and any caller) assumed result[0] was nearest. Fix that here.
    for (int i = 1; i < hits; i++)
    {
        SolRayResult key = result[i];
        int j = i - 1;
        while (j >= 0 && result[j].dist > key.dist)
        {
            result[j + 1] = result[j];
            j--;
        }
        result[j + 1] = key;
    }

    return hits;
}

static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Ray vs infinite cylinder of radius r along segment [a,b]. dir must be unit length.
static bool Ray_Intersect_Cylinder(vec3s O, vec3s D, float maxDist, vec3s a, vec3s b, float r,
                                    float *outT, float *outS)
{
    vec3s axis    = glms_vec3_sub(b, a);
    float axisLen = glms_vec3_norm(axis);
    if (axisLen < 1e-8f)
        return false;
    vec3s d = glms_vec3_scale(axis, 1.0f / axisLen);

    vec3s m  = glms_vec3_sub(O, a);
    float md = glms_vec3_dot(m, d);
    float dd = glms_vec3_dot(D, d);

    float qa = 1.0f - dd * dd; // D is unit
    float qb = 2.0f * (glms_vec3_dot(m, D) - md * dd);
    float qc = glms_vec3_dot(m, m) - md * md - r * r;

    if (fabsf(qa) < 1e-8f)
        return false; // ray parallel to edge axis -- vertex spheres cover this case

    float disc = qb * qb - 4.0f * qa * qc;
    if (disc < 0.0f)
        return false;

    float sqrtDisc = sqrtf(disc);
    float t0 = (-qb - sqrtDisc) / (2.0f * qa);
    float t1 = (-qb + sqrtDisc) / (2.0f * qa);
    float t  = (t0 > 1e-6f) ? t0 : t1;
    if (t <= 1e-6f || t > maxDist)
        return false;

    float s = (md + t * dd) / axisLen;
    if (s < 0.0f || s > 1.0f)
        return false;

    *outT = t;
    *outS = s;
    return true;
}

// Sphere-swept ray ("thick ray" / capsule) vs triangle.
bool Ray_Intersect_Tri_Thick(vec3s O, vec3s D, float maxDist, const SolTri *tri, float r,
                              float *outT, vec3s *outNorm)
{
    float bestT    = maxDist;
    bool  found    = false;
    vec3s bestNorm = {0};

    // --- Face (offset plane) test ---
    float faceSide   = glms_vec3_dot(glms_vec3_sub(O, tri->v0), tri->normal);
    float faceSign   = (faceSide >= 0.0f) ? r : -r;
    vec3s planeP0    = glms_vec3_add(tri->v0, glms_vec3_scale(tri->normal, faceSign));
    float denom      = glms_vec3_dot(D, tri->normal);

    if (fabsf(denom) > 1e-8f)
    {
        float t = glms_vec3_dot(glms_vec3_sub(planeP0, O), tri->normal) / denom;
        if (t > 1e-6f && t < bestT)
        {
            vec3s P     = glms_vec3_add(O, glms_vec3_scale(D, t));
            vec3s Pflat = glms_vec3_sub(P, glms_vec3_scale(tri->normal, faceSign));

            vec3s v0v1 = glms_vec3_sub(tri->v1, tri->v0);
            vec3s v0v2 = glms_vec3_sub(tri->v2, tri->v0);
            vec3s v0p  = glms_vec3_sub(Pflat, tri->v0);
            float d00 = glms_vec3_dot(v0v1, v0v1), d01 = glms_vec3_dot(v0v1, v0v2);
            float d11 = glms_vec3_dot(v0v2, v0v2), d20 = glms_vec3_dot(v0p, v0v1);
            float d21 = glms_vec3_dot(v0p, v0v2);
            float invDenom = 1.0f / (d00 * d11 - d01 * d01);
            float u = (d11 * d20 - d01 * d21) * invDenom;
            float v = (d00 * d21 - d01 * d20) * invDenom;

            if (u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f)
            {
                bestT    = t;
                bestNorm = (faceSide >= 0.0f) ? tri->normal : glms_vec3_scale(tri->normal, -1.0f);
                found    = true;
            }
        }
    }

    // --- Edge cylinder tests ---
    vec3s edges[3][2] = {{tri->v0, tri->v1}, {tri->v1, tri->v2}, {tri->v2, tri->v0}};
    for (int e = 0; e < 3; e++)
    {
        float t, s;
        if (Ray_Intersect_Cylinder(O, D, bestT, edges[e][0], edges[e][1], r, &t, &s))
        {
            vec3s axisP = glms_vec3_lerp(edges[e][0], edges[e][1], s);
            vec3s P     = glms_vec3_add(O, glms_vec3_scale(D, t));
            bestT    = t;
            bestNorm = glms_vec3_normalize(glms_vec3_sub(P, axisP));
            found    = true;
        }
    }

    // --- Vertex sphere tests ---
    vec3s verts[3] = {tri->v0, tri->v1, tri->v2};
    for (int v = 0; v < 3; v++)
    {
        float t; vec3s n;
        if (Ray_Intersect_Sphere(O, D, bestT, verts[v], r, &t, &n))
        {
            bestT    = t;
            bestNorm = n;
            found    = true;
        }
    }

    if (found)
    {
        *outT    = bestT;
        *outNorm = bestNorm;
    }
    return found;
}
int Sol_Body3_Spherecast(World *world, SolRay ray, float radius, SolRayResult *result, int max)
{
    if (!result || max <= 0 || ray.dist <= 0.0f)
        return 0;
    if (glms_vec3_dot(ray.dir, ray.dir) < 1e-12f)
        return 0;
    ray.dir = glms_vec3_normalize(ray.dir);

    SysPhysx *ws   = world->systems[WORLDSYS_BODY3];
    int       hits = 0;

    vec3s rayEnd = glms_vec3_add(ray.start, glms_vec3_scale(ray.dir, ray.dist));
    vec3s minP   = glms_vec3_sub(glms_vec3_minv(ray.start, rayEnd), (vec3s){radius, radius, radius});
    vec3s maxP   = glms_vec3_add(glms_vec3_maxv(ray.start, rayEnd), (vec3s){radius, radius, radius});

    SpatialGrid *grids[2]    = {&ws->static_group.spatial, &ws->dynamic_group.spatial};
    bool         isStatic[2] = {true, false};

    for (int g = 0; g < 2 && hits < max; g++)
    {
        SpatialGrid *grid = grids[g];
        if (grid->item_count == 0)
            continue;

        ivec3s minCell = SpatialGrid_WorldToCell(grid, minP);
        ivec3s maxCell = SpatialGrid_WorldToCell(grid, maxP);
        minCell.x = clampi(minCell.x, 0, grid->dims.x - 1); maxCell.x = clampi(maxCell.x, 0, grid->dims.x - 1);
        minCell.y = clampi(minCell.y, 0, grid->dims.y - 1); maxCell.y = clampi(maxCell.y, 0, grid->dims.y - 1);
        minCell.z = clampi(minCell.z, 0, grid->dims.z - 1); maxCell.z = clampi(maxCell.z, 0, grid->dims.z - 1);

        for (int z = minCell.z; z <= maxCell.z && hits < max; z++)
        for (int y = minCell.y; y <= maxCell.y && hits < max; y++)
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
                    if (entId == ray.ignoreEnt || Is_Already_Hit(result, hits, entId))
                        continue;

                    float tHit; vec3s normHit;
                    if (Ray_Intersect_Tri_Thick(ray.start, ray.dir, ray.dist, tri, radius, &tHit, &normHit))
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
                    if (entId == ray.ignoreEnt || Is_Already_Hit(result, hits, entId))
                        continue;

                    ScBody3 *body  = Sol_Comp_Get(world, entId, ScBody3);
                    ScXform *xform = Sol_Comp_Get(world, entId, ScXform);
                    if (!body || !xform || !(body->mask & ray.mask))
                        continue;

                    if (body->shape == SHAPE3_SPH)
                    {
                        float expandedRadius = body->dims.x + radius;
                        float tHit; vec3s normHit;
                        if (Ray_Intersect_Sphere(ray.start, ray.dir, ray.dist, xform->pos, expandedRadius, &tHit,
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
        }
    }

    for (int i = 1; i < hits; i++) // sort by distance, same as Sol_Raycast
    {
        SolRayResult key = result[i];
        int j = i - 1;
        while (j >= 0 && result[j].dist > key.dist)
        {
            result[j + 1] = result[j];
            j--;
        }
        result[j + 1] = key;
    }

    return hits;
}

float Sol_Body3_GetSpeed(World *world, int id)
{
    return glms_vec3_norm(Sol_Comp_Get(world, id, ScBody3)->vel);
}
vec3s Sol_Body3_GetDir(World *world, int id)
{
    return vecNorm(Sol_Comp_Get(world, id, ScBody3)->vel);
}