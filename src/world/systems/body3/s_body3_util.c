#include "s_body3.h"
#include "model.h"
#include "sol_math.h"
#include "world.h"
#include "profiler.h"

#include <omp.h>

typedef bool (*ShapePairTest)(World *world, int idA, int idB, SolContact *contact);
const ShapePairTest shape_pair_test[SHAPE3_CNT][SHAPE3_CNT] = {
    [SHAPE3_SPH][SHAPE3_SPH] = Collide_Sphere_Sphere,
    [SHAPE3_CAP][SHAPE3_CAP] = Collide_Capsule_Capsule,
    [SHAPE3_CAP][SHAPE3_SPH] = Collide_Capsule_Sphere,
    [SHAPE3_SPH][SHAPE3_CAP] = Collide_Sphere_Capsule,
};
typedef bool (*ShapeTriTest)(World *world, int idA, int idB, const SolTri *tri, SolContact *contact);
const ShapeTriTest shape_tri_test[SHAPE3_CNT] = {
    [SHAPE3_SPH] = Collide_Sphere_Tri,
    [SHAPE3_CAP] = Collide_Capsule_Tri,
};

static SolProfiler prof1 = {.name = "DynamicTables"};
static SolProfiler prof2 = {.name = "StaticTables"};

void Build_Tables(World *world, SysPhysx *sys, float fdt)
{
    int i;
    Prof_Begin(&prof1);
    solb_zero(sys->dynamic_group.aabb_scratch);
    SparseSet_ScBody3 *bodySet = Sol_Comp_Set(world, ScBody3);
    for (i = 0; i < bodySet->cnt; i++)
    {
        int id        = bodySet->dense[i];
        ScBody3 *body = &bodySet->data[i];
        Xform xform   = Xform_Get(world, id);
        SpatialAABB aabb;
        aabb.id  = id;
        aabb.min = vecSub(xform.pos, body->dims);
        aabb.max = vecAdd(xform.pos, body->dims);
        solb_push(sys->dynamic_group.aabb_scratch, aabb);
    }
    SpatialGrid_BuildFromAABBs(&sys->dynamic_group.spatial, sys->dynamic_group.aabb_scratch,
                               solb_count(sys->dynamic_group.aabb_scratch));
    Prof_EndEz(&prof1, true, fdt);

    SparseSet_ScStage *stageSet = Sol_Comp_Set(world, ScStage);
    bool stageDirty             = false;
    for (i = 0; i < stageSet->cnt; i++)
    {
        if (stageSet->data[i].isDirty)
        {
            stageDirty = true;
            break;
        }
    }
    if (stageDirty)
    {
        solb_set_count(sys->static_group.tris, 0);
        for (i = 0; i < stageSet->cnt; i++)
        {
            int id         = stageSet->dense[i];
            ScStage *stage = &stageSet->data[i];
            ScModel *model = Sol_Comp_Get(world, id, ScModel);
            if (!model)
                continue;
            int tricount = loaded_models[model->kind].tri_count;
            SolTri *tris = loaded_models[model->kind].tris;
            for (int t = 0; t < tricount; t++)
            {
                // SolTri tri = SolTri_GetWorldSpace(&tris[t], xform->rot, xform->pos, xform->sca);
                SolTri tri = tris[t];
                tri.entId  = id;
                solb_push(sys->static_group.tris, tri);
            }
            stage->isDirty = false;
        }
        sollog("WorldTri count:", solb_count(sys->static_group.tris));
        SpatialGrid_BuildFromTris(&sys->static_group.spatial, sys->static_group.tris,
                                  solb_count(sys->static_group.tris));
    }
}

void Resolve_Contact(World *world, int idA, int idB, SolContact *contact)
{
    ScBody3 *bodyA = Sol_Comp_Get(world, idA, ScBody3);
    ScBody3 *bodyB = Sol_Comp_Get(world, idB, ScBody3);

    float invMassA = bodyA ? bodyA->invMass : 0.0f;
    float invMassB = bodyB ? bodyB->invMass : 0.0f;

    float totalInvMass = invMassA + invMassB;
    if (totalInvMass <= 0.0f)
        return;

    vec3s velA = bodyA ? bodyA->vel : GLMS_VEC3_ZERO;
    vec3s velB = bodyB ? bodyB->vel : GLMS_VEC3_ZERO;

    vec3s relativeVel    = glms_vec3_sub(velA, velB);
    float velAlongNormal = glms_vec3_dot(relativeVel, contact->normal);

    // --- 1. Softened Positional Correction ---
    // Lower percentage (0.10f - 0.12f) prevents the "repel/bounce" feeling while
    // still resolving overlap over 3-5 frames.
    const float slack   = SOLVER_SLACK;
    const float percent = SOLVER_PERCENT;

    float pen = fmaxf(contact->penetration - slack, 0.0f);
    if (pen > 0.0f)
    {
        float corrMag    = (pen / totalInvMass) * percent;
        vec3s correction = glms_vec3_scale(contact->normal, corrMag);

        if (invMassA > 0.0f)
            world->xform.pos[idA] = glms_vec3_add(world->xform.pos[idA], glms_vec3_scale(correction, invMassA));

        if (invMassB > 0.0f)
            world->xform.pos[idB] = glms_vec3_sub(world->xform.pos[idB], glms_vec3_scale(correction, invMassB));
    }

    // --- 2. Velocity Projection (Sliding + Elastic Bounce) ---
    if (velAlongNormal >= 0.0f)
        return;

    float restA = bodyA ? bodyA->restitution : 0.0f;
    float restB = bodyB ? bodyB->restitution : 0.0f;

    // Standard mixing: fmaxf (or restA * restB) so bouncy objects bounce off non-bouncy ground
    float e = fmaxf(restA, restB);

// --- RESTITUTION THRESHOLD ---
// If the impact speed is low (e.g. under 0.5m/s - 1.0m/s or resting gravity),
// clamp restitution to 0 to prevent micro-bouncing/jitter while sliding.
#define SOLVER_RESTITUTION_THRESHOLD 1.0f

    if (fabsf(velAlongNormal) < SOLVER_RESTITUTION_THRESHOLD)
    {
        e = 0.0f;
    }

    float j       = -(1.0f + e) * velAlongNormal / totalInvMass;
    vec3s impulse = glms_vec3_scale(contact->normal, j);

    if (bodyA && invMassA > 0.0f)
        bodyA->vel = glms_vec3_add(bodyA->vel, glms_vec3_scale(impulse, invMassA));

    if (bodyB && invMassB > 0.0f)
        bodyB->vel = glms_vec3_sub(bodyB->vel, glms_vec3_scale(impulse, invMassB));
}

void Collisions_Static_Stage_Local(World *world, int idA, ScBody3 *body, vec3s min, vec3s max, StaticGroup *group,
                                   ThreadContactBuffer *contacts)
{
    ivec3s minCellA = SpatialGrid_WorldToCell(&group->spatial, min);
    ivec3s maxCellA = SpatialGrid_WorldToCell(&group->spatial, max);

    int gridDimX = group->spatial.dims.x;
    int gridDimY = group->spatial.dims.y;
    int gridDimZ = group->spatial.dims.z;

    minCellA.x = clampi(minCellA.x, 0, gridDimX - 1);
    maxCellA.x = clampi(maxCellA.x, 0, gridDimX - 1);
    minCellA.y = clampi(minCellA.y, 0, gridDimY - 1);
    maxCellA.y = clampi(maxCellA.y, 0, gridDimY - 1);
    minCellA.z = clampi(minCellA.z, 0, gridDimZ - 1);
    maxCellA.z = clampi(maxCellA.z, 0, gridDimZ - 1);

    for (int z = minCellA.z; z <= maxCellA.z; z++)
    {
        for (int y = minCellA.y; y <= maxCellA.y; y++)
        {
            for (int x = minCellA.x; x <= maxCellA.x; x++)
            {
                uint32_t cellIdx = SpatialGrid_GetCellIndex(&group->spatial, x, y, z);
                GridCell cell    = group->spatial.cells[cellIdx];

                for (uint32_t i = 0; i < cell.count; i++)
                {
                    uint32_t idx      = group->spatial.index_buffer[cell.offset + i];
                    const SolTri *tri = &group->tris[idx];

                    // --- Coordinate-Based Deduplication ---
                    // Compute triangle minimum bound (or use tri->min if precalculated)
                    vec3s triMin      = glms_vec3_minv(glms_vec3_minv(tri->a, tri->b), tri->c);
                    ivec3s minCellTri = SpatialGrid_WorldToCell(&group->spatial, triMin);

                    // Find the primary shared cell between Body A and the Triangle
                    int startX = minCellA.x > minCellTri.x ? minCellA.x : minCellTri.x;
                    int startY = minCellA.y > minCellTri.y ? minCellA.y : minCellTri.y;
                    int startZ = minCellA.z > minCellTri.z ? minCellA.z : minCellTri.z;

                    startX = clampi(startX, 0, gridDimX - 1);
                    startY = clampi(startY, 0, gridDimY - 1);
                    startZ = clampi(startZ, 0, gridDimZ - 1);

                    // Execute collision test ONLY when visiting the primary overlapping cell
                    if (x != startX || y != startY || z != startZ)
                        continue;

                    int idB = tri->entId;
                    if (body->ignoreEnt == idB)
                        continue;

                    SolContact contact;
                    if (shape_tri_test[body->shape] && shape_tri_test[body->shape](world, idA, idB, tri, &contact))
                    {
                        if (contacts->count < MAX_THREAD_CONTACTS)
                        {
                            contact.id  = idA;
                            contact.idB = idB;

                            contacts->contacts[contacts->count++] = contact;
                        }
                    }
                }
            }
        }
    }
}

void Collisions_Dynamic_Bodies_Local(World *world, int idA, ScBody3 *body, vec3s min, vec3s max, DynamicGroup *group,
                                     ThreadContactBuffer *contacts)
{
    ivec3s minCellA = SpatialGrid_WorldToCell(&group->spatial, min);
    ivec3s maxCellA = SpatialGrid_WorldToCell(&group->spatial, max);

    int gridDimX = group->spatial.dims.x;
    int gridDimY = group->spatial.dims.y;
    int gridDimZ = group->spatial.dims.z;

    minCellA.x = clampi(minCellA.x, 0, gridDimX - 1);
    maxCellA.x = clampi(maxCellA.x, 0, gridDimX - 1);
    minCellA.y = clampi(minCellA.y, 0, gridDimY - 1);
    maxCellA.y = clampi(maxCellA.y, 0, gridDimY - 1);
    minCellA.z = clampi(minCellA.z, 0, gridDimZ - 1);
    maxCellA.z = clampi(maxCellA.z, 0, gridDimZ - 1);

    for (int z = minCellA.z; z <= maxCellA.z; z++)
    {
        for (int y = minCellA.y; y <= maxCellA.y; y++)
        {
            for (int x = minCellA.x; x <= maxCellA.x; x++)
            {
                uint32_t cellIdx = SpatialGrid_GetCellIndex(&group->spatial, x, y, z);
                GridCell cell    = group->spatial.cells[cellIdx];

                for (uint32_t i = 0; i < cell.count; i++)
                {
                    uint32_t idB = group->spatial.index_buffer[cell.offset + i];
                    if (idA >= idB)
                        continue;

                    if (!Sol_Comp_Has(world, idB, ScBody3))
                        continue;
                    ScBody3 *other_body = Sol_Comp_Get(world, idB, ScBody3);

                    if (!((body->mask >> 16) & COLLAYER_TEAMZ || (other_body->mask >> 16 & COLLAYER_TEAMZ)) ||
                        body->ignoreEnt == idB || other_body->ignoreEnt == idA ||
                        !((body->mask >> 16) & other_body->mask) || !((other_body->mask >> 16) & body->mask))
                        continue;

                    // Compute minimum shared overlapping cell between A and B
                    vec3s minB      = glms_vec3_sub(world->xform.pos[idB], other_body->dims);
                    ivec3s minCellB = SpatialGrid_WorldToCell(&group->spatial, minB);

                    int startX = minCellA.x > minCellB.x ? minCellA.x : minCellB.x;
                    int startY = minCellA.y > minCellB.y ? minCellA.y : minCellB.y;
                    int startZ = minCellA.z > minCellB.z ? minCellA.z : minCellB.z;

                    startX = clampi(startX, 0, gridDimX - 1);
                    startY = clampi(startY, 0, gridDimY - 1);
                    startZ = clampi(startZ, 0, gridDimZ - 1);

                    // Skip duplicate pairs across adjacent cells
                    if (x != startX || y != startY || z != startZ)
                        continue;

                    SolContact contact;
                    if (shape_pair_test[body->shape][other_body->shape] &&
                        shape_pair_test[body->shape][other_body->shape](world, idA, idB, &contact))
                    {
                        if (contacts->count < MAX_THREAD_CONTACTS)
                        {
                            contact.id                            = idA;
                            contact.idB                           = idB;
                            contacts->contacts[contacts->count++] = contact;
                        }
                    }
                }
            }
        }
    }
}

bool Collide_Sphere_Sphere(World *world, int idA, int idB, SolContact *contact)
{
    ScBody3 *bodyA = Sol_Comp_Get(world, idA, ScBody3);
    ScBody3 *bodyB = Sol_Comp_Get(world, idB, ScBody3);
    Xform xformA   = Xform_Get(world, idA);
    Xform xformB   = Xform_Get(world, idB);
    if (!bodyA || !bodyB)
        return false;

    vec3s delta     = glms_vec3_sub(xformA.pos, xformB.pos);
    float distSq    = glms_vec3_dot(delta, delta);
    float radiusSum = bodyA->dims.x + bodyB->dims.x;

    if (distSq >= (radiusSum * radiusSum))
        return false;

    float distance = sqrtf(distSq);

    if (distance < 0.0001f)
    {
        // Safe default normal when spheres are centered at identical coordinates
        contact->normal      = (vec3s){0.0f, 1.0f, 0.0f};
        contact->penetration = radiusSum;
        contact->pos         = xformB.pos;
    }
    else
    {
        contact->normal      = glms_vec3_scale(delta, 1.0f / distance);
        contact->penetration = radiusSum - distance;
        contact->pos =
            glms_vec3_add(xformB.pos, glms_vec3_scale(contact->normal, radiusSum - 0.5f * contact->penetration));
    }

    return true;
}

bool Collide_Capsule_Capsule(World *world, int idA, int idB, SolContact *hit)
{
    ScBody3 *bodyA = Sol_Comp_Get(world, idA, ScBody3);
    ScBody3 *bodyB = Sol_Comp_Get(world, idB, ScBody3);
    Xform xformA   = Xform_Get(world, idA);
    Xform xformB   = Xform_Get(world, idB);

    float aRadius   = bodyA->dims.x;
    float aHalfSpan = (bodyA->dims.y * 0.5f) - aRadius;
    if (aHalfSpan < 0.0f)
        aHalfSpan = 0.0f;

    float bRadius   = bodyB->dims.x;
    float bHalfSpan = (bodyB->dims.y * 0.5f) - bRadius;
    if (bHalfSpan < 0.0f)
        bHalfSpan = 0.0f;

    vec3s aTop    = {{xformA.pos.x, xformA.pos.y + aHalfSpan, xformA.pos.z}};
    vec3s aBottom = {{xformA.pos.x, xformA.pos.y - aHalfSpan, xformA.pos.z}};

    vec3s bTop    = {{xformB.pos.x, xformB.pos.y + bHalfSpan, xformB.pos.z}};
    vec3s bBottom = {{xformB.pos.x, xformB.pos.y - bHalfSpan, xformB.pos.z}};

    // Find closest points between the two spine segments
    vec3s closestA, closestB;
    Closest_Points_Segment_Segment(aBottom, aTop, bBottom, bTop, &closestA, &closestB);

    // Sphere-sphere from these closest points
    vec3s delta     = glms_vec3_sub(closestA, closestB);
    float distSq    = glms_vec3_dot(delta, delta);
    float radiusSum = aRadius + bRadius;

    if (distSq >= (radiusSum * radiusSum) || distSq < 0.0001f)
        return false;

    float distance = sqrtf(distSq);

    hit->normal      = glms_vec3_scale(delta, 1.0f / distance);
    hit->penetration = radiusSum - distance;
    hit->pos         = glms_vec3_add(closestB, glms_vec3_scale(hit->normal, bRadius));

    return true;
}
bool Collide_Sphere_Capsule(World *world, int idA, int idB, SolContact *hit)
{
    ScBody3 *bodyA = Sol_Comp_Get(world, idA, ScBody3);
    ScBody3 *bodyB = Sol_Comp_Get(world, idB, ScBody3);
    Xform xformA   = Xform_Get(world, idA);
    Xform xformB   = Xform_Get(world, idB);

    float aRadius = bodyA->dims.x;

    float bRadius   = bodyB->dims.x;
    float bHalfSpan = (bodyB->dims.y * 0.5f) - bRadius;
    if (bHalfSpan < 0.0f)
        bHalfSpan = 0.0f;

    vec3s bTop    = {{xformB.pos.x, xformB.pos.y + bHalfSpan, xformB.pos.z}};
    vec3s bBottom = {{xformB.pos.x, xformB.pos.y - bHalfSpan, xformB.pos.z}};

    // Find closest point on capsule B's spine to sphere A's center
    vec3s closestB = Closest_Point_Segment_Point(bBottom, bTop, xformA.pos);

    // Sphere-sphere collision test between Sphere A and closest point on Capsule B
    vec3s delta     = glms_vec3_sub(xformA.pos, closestB);
    float distSq    = glms_vec3_dot(delta, delta);
    float radiusSum = aRadius + bRadius;

    if (distSq >= (radiusSum * radiusSum) || distSq < 0.0001f)
        return false;

    float distance = sqrtf(distSq);

    hit->normal      = glms_vec3_scale(delta, 1.0f / distance); // Points from B (Capsule) to A (Sphere)
    hit->penetration = radiusSum - distance;
    hit->pos         = glms_vec3_add(closestB, glms_vec3_scale(hit->normal, bRadius));

    return true;
}
bool Collide_Capsule_Sphere(World *world, int idA, int idB, SolContact *hit)
{
    ScBody3 *bodyA = Sol_Comp_Get(world, idA, ScBody3);
    ScBody3 *bodyB = Sol_Comp_Get(world, idB, ScBody3);
    Xform xformA   = Xform_Get(world, idA);
    Xform xformB   = Xform_Get(world, idB);

    float aRadius   = bodyA->dims.x;
    float aHalfSpan = (bodyA->dims.y * 0.5f) - aRadius;
    if (aHalfSpan < 0.0f)
        aHalfSpan = 0.0f;

    float bRadius = bodyB->dims.x;

    vec3s aTop    = {{xformA.pos.x, xformA.pos.y + aHalfSpan, xformA.pos.z}};
    vec3s aBottom = {{xformA.pos.x, xformA.pos.y - aHalfSpan, xformA.pos.z}};

    // Find closest point on capsule A's spine to sphere B's center
    vec3s closestA = Closest_Point_Segment_Point(aBottom, aTop, xformB.pos);

    // Sphere-sphere collision test between closest point on Capsule A and Sphere B
    vec3s delta     = glms_vec3_sub(closestA, xformB.pos);
    float distSq    = glms_vec3_dot(delta, delta);
    float radiusSum = aRadius + bRadius;

    if (distSq >= (radiusSum * radiusSum) || distSq < 0.0001f)
        return false;

    float distance = sqrtf(distSq);

    hit->normal      = glms_vec3_scale(delta, 1.0f / distance); // Points from B (Sphere) to A (Capsule)
    hit->penetration = radiusSum - distance;
    hit->pos         = glms_vec3_add(xformB.pos, glms_vec3_scale(hit->normal, bRadius));

    return true;
}
bool Collide_Box_Box(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}
bool Collide_Box_Sphere(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}
bool Collide_Sphere_Box(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}

bool Collide_Sphere_Tri(World *world, int idA, int idB, const SolTri *tri, SolContact *contact)
{
    ScBody3 *body3 = Sol_Comp_Get(world, idA, ScBody3);
    Xform xform    = Xform_Get(world, idA);
    vec3s closestP = ClosestPointOnTriangle(xform.pos, tri->a, tri->b, tri->c);
    vec3s delta    = glms_vec3_sub(xform.pos, closestP);
    float distSq   = glms_vec3_dot(delta, delta);

    if (distSq >= body3->dims.x * body3->dims.x)
        return false;

    float dist        = sqrtf(distSq);
    vec3s normal      = dist > 0.0001f ? glms_vec3_scale(delta, 1.0f / dist) : tri->normal;
    float penetration = body3->dims.x - dist;

    contact->pos         = closestP;
    contact->penetration = penetration;
    contact->normal      = normal;

    return true;
}

bool Collide_Capsule_Tri(World *world, int idA, int idB, const SolTri *tri, SolContact *hit)
{
    ScBody3 *body = Sol_Comp_Get(world, idA, ScBody3);
    Xform xform   = Xform_Get(world, idA);

    float radius     = body->dims.x;
    float halfHeight = fmaxf(0.0f, body->dims.y * 0.5f - radius);

    // 1. Broadphase Bounding Sphere Early-Out
    float maxReach = halfHeight + radius;
    float maxDist  = tri->bounds + maxReach;
    if (glms_vec3_norm2(glms_vec3_sub(xform.pos, tri->center)) > (maxDist * maxDist))
        return false;

    // Segment end points (capsule center line)
    vec3s segA = xform.pos;
    vec3s segB = xform.pos;
    segA.y += halfHeight;
    segB.y -= halfHeight;

    vec3s bestCapPoint = segA;
    vec3s bestTriPoint = tri->a;
    float bestDistSq   = 1e30f;

    // 2. Fast-Path: Test capsule segment against triangle plane face
    vec3s triNorm = tri->normal;
    float dA      = glms_vec3_dot(glms_vec3_sub(segA, tri->a), triNorm);
    float dB      = glms_vec3_dot(glms_vec3_sub(segB, tri->a), triNorm);

    // Find point on segment closest to plane face
    float tPlane = 0.5f;
    if (fabsf(dA - dB) > 1e-6f)
        tPlane = fmaxf(0.0f, fminf(1.0f, dA / (dA - dB)));

    vec3s planeCapPt  = glms_vec3_add(segA, glms_vec3_scale(glms_vec3_sub(segB, segA), tPlane));
    float distToPlane = glms_vec3_dot(glms_vec3_sub(planeCapPt, tri->a), triNorm);
    vec3s planeProjPt = glms_vec3_sub(planeCapPt, glms_vec3_scale(triNorm, distToPlane));

    // Barycentric test to check if planeProjPt is inside triangle boundaries
    vec3s v0 = glms_vec3_sub(tri->b, tri->a);
    vec3s v1 = glms_vec3_sub(tri->c, tri->a);
    vec3s v2 = glms_vec3_sub(planeProjPt, tri->a);

    float d00 = glms_vec3_dot(v0, v0);
    float d01 = glms_vec3_dot(v0, v1);
    float d11 = glms_vec3_dot(v1, v1);
    float d20 = glms_vec3_dot(v2, v0);
    float d21 = glms_vec3_dot(v2, v1);

    float invDenom = 1.0f / (d00 * d11 - d01 * d01 + 1e-8f);
    float u        = (d11 * d20 - d01 * d21) * invDenom;
    float v        = (d00 * d21 - d01 * d20) * invDenom;

    if (u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f)
    {
        // Hit flat face directly
        bestCapPoint = planeCapPt;
        bestTriPoint = planeProjPt;
        bestDistSq   = distToPlane * distToPlane;
    }
    else
    {
        // 3. Fallback Path: Test segment against 3 triangle edges
        vec3s verts[3] = {tri->a, tri->b, tri->c};
        for (int i = 0; i < 3; i++)
        {
            vec3s cpCap, cpEdge;
            Closest_Points_Segment_Segment(segA, segB, verts[i], verts[(i + 1) % 3], &cpCap, &cpEdge);
            float dSq = glms_vec3_norm2(glms_vec3_sub(cpCap, cpEdge));

            if (dSq < bestDistSq)
            {
                bestDistSq   = dSq;
                bestCapPoint = cpCap;
                bestTriPoint = cpEdge;
            }
        }
    }

    // 4. Distance threshold check
    float radiusSq = radius * radius;
    if (bestDistSq >= radiusSq)
        return false;

    float dist  = sqrtf(bestDistSq);
    hit->normal = (dist > 1e-4f) ? glms_vec3_scale(glms_vec3_sub(bestCapPoint, bestTriPoint), 1.0f / dist) : triNorm;

    hit->penetration = radius - dist;
    hit->pos         = bestTriPoint;

    return true;
}

bool SphereCast_VsSphere(vec3s rayPos, vec3s rayDir, float rayLen, float castRadius, vec3s spherePos,
                         float sphereRadius, SolRayResult *hit)
{
    float combinedR = castRadius + sphereRadius;

    vec3s m = glms_vec3_sub(rayPos, spherePos);
    float b = glms_vec3_dot(m, rayDir);
    float c = glms_vec3_dot(m, m) - combinedR * combinedR;

    // Ray starts already inside? Hit at start.
    if (c < 0.0f)
    {
        hit->pos  = rayPos;
        hit->dist = 0.0f;
        hit->norm = glms_vec3_normalize(glms_vec3_sub(rayPos, spherePos));
        return true;
    }

    // Ray pointing away from sphere
    if (b > 0.0f)
        return false;

    float discr = b * b - c;
    if (discr < 0.0f)
        return false;

    float t = -b - sqrtf(discr);
    if (t > rayLen)
        return false;
    if (t < 0.0f)
        t = 0.0f;

    hit->dist        = t;
    vec3s castCenter = glms_vec3_add(rayPos, glms_vec3_scale(rayDir, t));
    hit->pos         = castCenter; // could refine to surface
    hit->norm        = glms_vec3_normalize(glms_vec3_sub(castCenter, spherePos));
    return true;
}

bool SphereCast_VsCapsule(vec3s rayPos, vec3s rayDir, float rayLen, float castRadius, vec3s capPos, float capRadius,
                          float capHeight, SolRayResult *hit)
{
    float combinedR = castRadius + capRadius;
    float halfSpan  = capHeight * 0.5f - capRadius;
    if (halfSpan < 0)
        halfSpan = 0;

    vec3s spineTop    = {{capPos.x, capPos.y + halfSpan, capPos.z}};
    vec3s spineBottom = {{capPos.x, capPos.y - halfSpan, capPos.z}};
    vec3s rayEnd      = glms_vec3_add(rayPos, glms_vec3_scale(rayDir, rayLen));

    vec3s closestRay, closestSpine;
    Closest_Points_Segment_Segment(rayPos, rayEnd, spineBottom, spineTop, &closestRay, &closestSpine);

    vec3s delta  = glms_vec3_sub(closestRay, closestSpine);
    float distSq = glms_vec3_dot(delta, delta);

    if (distSq >= combinedR * combinedR)
        return false;

    // Approximate hit distance as distance from rayPos to closestRay
    float t = glms_vec3_norm(glms_vec3_sub(closestRay, rayPos));

    hit->dist = t;
    hit->pos  = closestSpine;
    hit->norm = (distSq > 0.0001f) ? glms_vec3_normalize(delta) : (vec3s){{0, 1, 0}};
    return true;
}

// static inline bool SweptSphere_Tri_Test(vec3s start, vec3s dir, float radius, const SolTri *tri, SolSweptHit *outHit)
// {
//     vec3s edge1 = glms_vec3_sub(tri->b, tri->a);
//     vec3s edge2 = glms_vec3_sub(tri->c, tri->a);
//     vec3s N     = glms_vec3_normalize(glms_vec3_cross(edge1, edge2));

//     // Distance from sphere start center to plane
//     float distToPlane = glms_vec3_dot(glms_vec3_sub(start, tri->a), N);
//     float denom       = glms_vec3_dot(dir, N);

//     // Moving parallel to or away from the front face
//     if (denom >= -1e-6f)
//         return false;

//     // Time when sphere surface contacts plane: (dist - radius) / -denom
//     float t_plane = (distToPlane - radius) / -denom;

//     if (t_plane < 0.0f || t_plane > 1.0f)
//         return false;

//     // Projected impact point on plane
//     vec3s planeHit = glms_vec3_add(start, glms_vec3_scale(dir, t_plane));
//     vec3s contactP = glms_vec3_sub(planeHit, glms_vec3_scale(N, radius));

//     // --- Barycentric Test on Plane Contact Point ---
//     vec3s v0 = glms_vec3_sub(tri->c, tri->a);
//     vec3s v1 = glms_vec3_sub(tri->b, tri->a);
//     vec3s v2 = glms_vec3_sub(contactP, tri->a);

//     float dot00 = glms_vec3_dot(v0, v0);
//     float dot01 = glms_vec3_dot(v0, v1);
//     float dot02 = glms_vec3_dot(v0, v2);
//     float dot11 = glms_vec3_dot(v1, v1);
//     float dot12 = glms_vec3_dot(v1, v2);

//     float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
//     float u        = (dot11 * dot02 - dot01 * dot12) * invDenom;
//     float v        = (dot00 * dot12 - dot01 * dot02) * invDenom;

//     // Hit inside triangle face
//     if (u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f)
//     {
//         outHit->hit    = true;
//         outHit->t      = t_plane;
//         outHit->point  = contactP;
//         outHit->normal = N;
//         return true;
//     }

//     return false;
// }

bool Is_Already_Hit(const SolRayResult *results, int hitCount, int entId)
{
    for (int i = 0; i < hitCount; i++)
    {
        if (results[i].entId == entId)
            return true;
    }
    return false;
}
// Ray vs Triangle Intersection (Möller–Trumbore)
bool Ray_Intersect_Tri(vec3s origin, vec3s dir, float maxDist, const SolTri *tri, float *outT, vec3s *outNorm)
{
    const float EPS = 0.000001f;
    vec3s e1        = glms_vec3_sub(tri->v1, tri->v0);
    vec3s e2        = glms_vec3_sub(tri->v2, tri->v0);
    vec3s h         = glms_vec3_cross(dir, e2);
    float a         = glms_vec3_dot(e1, h);

    if (a > -EPS && a < EPS)
        return false;

    float f = 1.0f / a;
    vec3s s = glms_vec3_sub(origin, tri->v0);
    float u = f * glms_vec3_dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    vec3s q = glms_vec3_cross(s, e1);
    float v = f * glms_vec3_dot(dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    float t = f * glms_vec3_dot(e2, q);
    if (t > EPS && t <= maxDist)
    {
        *outT    = t;
        *outNorm = glms_vec3_normalize(glms_vec3_cross(e1, e2));
        return true;
    }
    return false;
}
// Ray vs Sphere Intersection
bool Ray_Intersect_Sphere(vec3s origin, vec3s dir, float maxDist, vec3s center, float radius, float *outT,
                          vec3s *outNorm)
{
    vec3s oc = glms_vec3_sub(origin, center);
    float b  = glms_vec3_dot(oc, dir);
    float c  = glms_vec3_dot(oc, oc) - (radius * radius);

    if (c > 0.0f && b > 0.0f)
        return false;

    float discr = b * b - c;
    if (discr < 0.0f)
        return false;

    float t = -b - sqrtf(discr);
    if (t < 0.0f)
        t = -b + sqrtf(discr);

    if (t > 0.0001f && t <= maxDist)
    {
        *outT        = t;
        vec3s hitPos = glms_vec3_add(origin, glms_vec3_scale(dir, t));
        *outNorm     = glms_vec3_normalize(glms_vec3_sub(hitPos, center));
        return true;
    }
    return false;
}

// Ray vs infinite cylinder of radius r along segment [a,b]. dir must be unit length.
bool Ray_Intersect_Cylinder(vec3s O, vec3s D, float maxDist, vec3s a, vec3s b, float r, float *outT, float *outS)
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
    float t0       = (-qb - sqrtDisc) / (2.0f * qa);
    float t1       = (-qb + sqrtDisc) / (2.0f * qa);
    float t        = (t0 > 1e-6f) ? t0 : t1;
    if (t <= 1e-6f || t > maxDist)
        return false;

    float s = (md + t * dd) / axisLen;
    if (s < 0.0f || s > 1.0f)
        return false;

    *outT = t;
    *outS = s;
    return true;
}

// Ray vs Capsule (cylinder body + two hemispherical caps).
bool Ray_Intersect_Capsule(vec3s O, vec3s D, float maxDist, vec3s top, vec3s bottom, float radius, float *outT,
                           vec3s *outNorm)
{
    float bestT    = maxDist;
    bool found     = false;
    vec3s bestNorm = {0};

    float t, s;
    if (Ray_Intersect_Cylinder(O, D, bestT, top, bottom, radius, &t, &s))
    {
        vec3s axisP = glms_vec3_lerp(top, bottom, s);
        vec3s P     = glms_vec3_add(O, glms_vec3_scale(D, t));
        bestT       = t;
        bestNorm    = glms_vec3_normalize(glms_vec3_sub(P, axisP));
        found       = true;
    }

    vec3s caps[2] = {top, bottom};
    for (int i = 0; i < 2; i++)
    {
        float tCap;
        vec3s nCap;
        if (Ray_Intersect_Sphere(O, D, bestT, caps[i], radius, &tCap, &nCap))
        {
            bestT    = tCap;
            bestNorm = nCap;
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

// Sphere-swept ray ("thick ray" / capsule) vs triangle.
bool Ray_Intersect_Tri_Thick(vec3s O, vec3s D, float maxDist, const SolTri *tri, float r, float *outT, vec3s *outNorm)
{
    float bestT    = maxDist;
    bool found     = false;
    vec3s bestNorm = {0};

    // --- Face (offset plane) test ---
    float faceSide = glms_vec3_dot(glms_vec3_sub(O, tri->v0), tri->normal);
    float faceSign = (faceSide >= 0.0f) ? r : -r;
    vec3s planeP0  = glms_vec3_add(tri->v0, glms_vec3_scale(tri->normal, faceSign));
    float denom    = glms_vec3_dot(D, tri->normal);

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
            float d21      = glms_vec3_dot(v0p, v0v2);
            float invDenom = 1.0f / (d00 * d11 - d01 * d01);
            float u        = (d11 * d20 - d01 * d21) * invDenom;
            float v        = (d00 * d21 - d01 * d20) * invDenom;

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
            bestT       = t;
            bestNorm    = glms_vec3_normalize(glms_vec3_sub(P, axisP));
            found       = true;
        }
    }

    // --- Vertex sphere tests ---
    vec3s verts[3] = {tri->v0, tri->v1, tri->v2};
    for (int v = 0; v < 3; v++)
    {
        float t;
        vec3s n;
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
