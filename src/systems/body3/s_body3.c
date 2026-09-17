/*
 * File: s_body3.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-13
 *
 */
#include "world.h"
#include "sol_math.h"
#include "spatial_grid.h"
#include "profiler.h"
#include "model.h"
#include "sol_user.h"
#include <omp.h>

#define TERMINAL_VELOCITY -100.0f

static inline bool Sphere_Overlap_Capsule(vec3s center, float radius, vec3s top, vec3s bottom, float capRadius,
                                          float *outDist, vec3s *outNorm)
{
    vec3s closest  = ClosestPointOnSegment(center, top, bottom);
    vec3s delta    = glms_vec3_sub(center, closest);
    float distSq   = glms_vec3_dot(delta, delta);
    float combined = radius + capRadius;

    if (distSq >= combined * combined)
        return false;

    float dist = sqrtf(distSq);
    *outDist   = dist;
    *outNorm   = dist > 0.0001f ? glms_vec3_scale(delta, 1.0f / dist) : (vec3s){0.0f, 1.0f, 0.0f};
    return true;
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

bool Collide_Sphere_Sphere(World *world, ScBody3 *bodyA, Xform xformA, ScBody3 *bodyB, Xform xformB,
                           SolContact *contact)
{
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

bool Collide_Capsule_Capsule(World *world, ScBody3 *bodyA, Xform xformA, ScBody3 *bodyB, Xform xformB,
                             SolContact *contact)
{
    float aRadius   = bodyA->dims.x;
    float aHalfSpan = (bodyA->dims.y) - aRadius;
    if (aHalfSpan < 0.0f)
        aHalfSpan = 0.0f;

    float bRadius   = bodyB->dims.x;
    float bHalfSpan = (bodyB->dims.y) - bRadius;
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

    contact->normal      = glms_vec3_scale(delta, 1.0f / distance);
    contact->penetration = radiusSum - distance;
    contact->pos         = glms_vec3_add(closestB, glms_vec3_scale(contact->normal, bRadius));

    return true;
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

bool Collide_Capsule_Tri(World *world, int idA, int idB, const SolTri *tri, SolContact *contact)
{
    ScBody3 *body = Sol_Comp_Get(world, idA, ScBody3);
    Xform xform   = Xform_Get(world, idA);

    float radius        = body->dims.x;
    float segHalfHeight = fmaxf(0.0f, body->dims.y - radius);

    // 1. Broadphase Bounding Sphere Early-Out
    float maxReach = body->dims.y;
    float maxDist  = tri->bounds + maxReach;
    if (glms_vec3_norm2(glms_vec3_sub(xform.pos, tri->center)) > (maxDist * maxDist))
        return false;

    // Segment end points (capsule center line)
    vec3s segA = xform.pos;
    vec3s segB = xform.pos;
    segA.y += segHalfHeight;
    segB.y -= segHalfHeight;

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

    float dist = sqrtf(bestDistSq);
    contact->normal =
        (dist > 1e-4f) ? glms_vec3_scale(glms_vec3_sub(bestCapPoint, bestTriPoint), 1.0f / dist) : triNorm;

    contact->penetration = radius - dist;
    contact->pos         = bestTriPoint;

    return true;
}

bool Collide_Capsule_Sphere(World *world, ScBody3 *bodyA, Xform xformA, ScBody3 *bodyB, Xform xformB,
                            SolContact *contact)
{
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

    contact->normal      = glms_vec3_scale(delta, 1.0f / distance); // Points from B (Sphere) to A (Capsule)
    contact->penetration = radiusSum - distance;
    contact->pos         = glms_vec3_add(xformB.pos, glms_vec3_scale(contact->normal, bRadius));

    return true;
}

bool Collide_Sphere_Capsule(World *world, ScBody3 *bodyA, Xform xformA, ScBody3 *bodyB, Xform xformB,
                            SolContact *contact)
{
    if (Collide_Capsule_Sphere(world, bodyB, xformB, bodyA, xformA, contact))
    {
        contact->normal = vecSca(contact->normal, -1.0f);
        return true;
    }
    return false;
}

void Resolve_Contact(World *world, SolContact contact)
{
    int idA        = contact.idA;
    int idB        = contact.idB;
    ScBody3 *bodyA = Sol_Comp_Get(world, idA, ScBody3);
    ScBody3 *bodyB = Sol_Comp_Get(world, idB, ScBody3);
    bool sensorA = bodyA ? bodyA->is_sensor : false;
    bool sensorB = bodyB ? bodyB->is_sensor : false;
    if (sensorA || sensorB)
        return;

    float invMassA = bodyA ? bodyA->invMass : 0.0f;
    float invMassB = bodyB ? bodyB->invMass : 0.0f;

    float totalInvMass = invMassA + invMassB;
    if (totalInvMass <= 0.0f)
        return;

    vec3s velA = bodyA ? bodyA->vel : GLMS_VEC3_ZERO;
    vec3s velB = bodyB ? bodyB->vel : GLMS_VEC3_ZERO;

    vec3s relativeVel    = glms_vec3_sub(velA, velB);
    float velAlongNormal = glms_vec3_dot(relativeVel, contact.normal);

    const float slack   = 0.01f;
    const float percent = 0.5f;

    float pen = fmaxf(contact.penetration - slack, 0.0f);
    if (pen > 0.0f)
    {
        float corrMag    = (pen / totalInvMass) * percent;
        vec3s correction = glms_vec3_scale(contact.normal, corrMag);

        if (invMassA > 0.0f)
            world->xform.pos[idA] = glms_vec3_add(world->xform.pos[idA], glms_vec3_scale(correction, invMassA));

        if (invMassB > 0.0f)
            world->xform.pos[idB] = glms_vec3_sub(world->xform.pos[idB], glms_vec3_scale(correction, invMassB));
    }

    if (velAlongNormal >= 0.0f)
        return;

    float restA = bodyA ? bodyA->restitution : 0.0f;
    float restB = bodyB ? bodyB->restitution : 0.0f;

    float e = fmaxf(restA, restB);

    float j       = -(1.0f + e) * velAlongNormal / totalInvMass;
    vec3s impulse = glms_vec3_scale(contact.normal, j);

    if (bodyA && invMassA > 0.0f)
        bodyA->vel = glms_vec3_add(bodyA->vel, glms_vec3_scale(impulse, invMassA));

    if (bodyB && invMassB > 0.0f)
        bodyB->vel = glms_vec3_sub(bodyB->vel, glms_vec3_scale(impulse, invMassB));
}

typedef bool (*ShapePairTest)(World *world, ScBody3 *, Xform, ScBody3 *, Xform, SolContact *contact);
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

// typedef bool (*RayShapeTest)(World *world, int idA, int idB, SolContact *contact);
// const ShapeTriTest ray_shape_test[SHAPE3_CNT] = {
//     [SHAPE3_SPH] = Ray_Intersect_Sphere,
//     [SHAPE3_CAP] = Ray_Intersect_Capsule,
// };

static SolProfiler prof_body3       = {.name = "Body3"};
static SolProfiler prof_dynamic     = {.name = "Dynamic"};
static SolProfiler prof_static      = {.name = "StaticTable"};
static SolProfiler prof_static_test = {.name = "StaticTest"};

void Body3_Update(World *world)
{
    Prof_Begin(&prof_body3);
    float fdt = world->timestep;
    int i, body_count;

    SparseSet_ScBody3 *set = Sol_Comp_Set(world, ScBody3);
    for (i = set->cnt; i-- > 0;)
    {
        int id         = set->dense[i];
        ScBody3 *body3 = &set->data[i];
        if (body3->flag_destroy)
            Sol_Comp_Rem(world, id, ScBody3);
    }
    body_count = set->cnt;

    SlSpatial *spatial = Sol_Comp_Get(world, 0, SlSpatial); // world->singles[SINGLE_SPATIAL];
    solb_set_count(spatial->contacts, 0);
    for (i = body_count; i-- > 0;)
    {
        int id         = set->dense[i];
        ScBody3 *body3 = &set->data[i];

        XformP xform   = Xform_GetP(world, id);
        body3->vel     = glms_vec3_scale(body3->vel, 0.999f);
        vec3s accel    = body3->vel.y < TERMINAL_VELOCITY ? GLMS_VEC3_ZERO : body3->gravity;
        accel          = glms_vec3_add(accel, body3->force);
        accel          = glms_vec3_add(accel, body3->impulse);
        body3->impulse = (vec3s){0};
        body3->vel     = glms_vec3_add(body3->vel, glms_vec3_scale(accel, fdt));
        *xform.pos     = vecAdd(*xform.pos, vecSca(body3->vel, fdt));
    }

    solb_reserve(spatial->build_ids, body_count);
    solb_set_count(spatial->build_ids, body_count);
    solb_reserve(spatial->build_mins, body_count);
    solb_set_count(spatial->build_mins, body_count);
    solb_reserve(spatial->build_maxs, body_count);
    solb_set_count(spatial->build_maxs, body_count);

    for (i = body_count; i-- > 0;)
    {
        int id         = set->dense[i];
        ScBody3 *body3 = &set->data[i];

        spatial->build_ids[i]  = id;
        spatial->build_mins[i] = glms_vec3_sub(world->xform.pos[id], body3->dims);
        spatial->build_maxs[i] = glms_vec3_add(world->xform.pos[id], body3->dims);
    }

    SpatialGrid_Build(spatial->grid_dynamic, spatial->build_ids, spatial->build_mins, spatial->build_maxs,
                      solb_count(spatial->build_ids));

    bool stage_dirty             = false;
    SparseSet_ScStage *set_stage = Sol_Comp_Set(world, ScStage);
    int stage_count              = set_stage->cnt;
    for (i = 0; i < stage_count; i++)
    {
        int id         = set_stage->dense[i];
        ScStage *stage = &set_stage->data[i];
        if (stage->isDirty)
            stage_dirty = true;
    }

    if (stage_dirty)
    {
        solb_set_count(spatial->build_ids, 0);
        solb_set_count(spatial->build_mins, 0);
        solb_set_count(spatial->build_maxs, 0);
        for (i = 0; i < stage_count; i++)
        {
            int id         = set_stage->dense[i];
            ScStage *stage = &set_stage->data[i];
            stage->isDirty = false;
            Xform xform    = Xform_Get(world, id);

            if (Sol_Comp_Has(world, id, ScModel))
            {
                ScModel *model     = Sol_Comp_Get(world, id, ScModel);
                const SolTri *tris = loaded_models[model->kind].tris;
                int tri_count      = loaded_models[model->kind].tri_count;
                for (int j = 0; j < tri_count; j++)
                {
                    const SolTri tri = SolTri_GetWorldSpace(&tris[j], xform.rot, xform.pos, xform.sca);
                    solb_push(spatial->tris_static, tri);
                    vec3s tri_min = glms_vec3_minv(glms_vec3_minv(tri.a, tri.b), tri.c);
                    vec3s tri_max = glms_vec3_maxv(glms_vec3_maxv(tri.a, tri.b), tri.c);

                    // vec3s pos         = glms_vec3_scale(glms_vec3_add(tri_min, tri_max), 0.5f);
                    // vec3s half_extent = glms_vec3_scale(glms_vec3_sub(tri_max, tri_min), 0.5f);
                    solb_push(spatial->build_ids, SPATIAL_PACK_ID(id, solb_count(spatial->tris_static) - 1));
                    solb_push(spatial->build_mins, tri_min);
                    solb_push(spatial->build_maxs, tri_max);
                }
            }
        }
        SpatialGrid_Build(spatial->grid_static, spatial->build_ids, spatial->build_mins, spatial->build_maxs,
                          solb_count(spatial->build_ids));
    }

#pragma omp parallel if (body_count > 100)
    {
        int thread_num                 = omp_get_thread_num();
        ThreadContactBuffer *local_buf = &spatial->threadContacts[thread_num];
        solb_set_count(local_buf->contacts, 0);
        IdBuffer *id_buf = &spatial->threadIds[thread_num];
#pragma omp for schedule(dynamic)
        for (i = body_count - 1; i >= 0; i--)
        {
            int id        = set->dense[i];
            ScBody3 *body = &set->data[i];
            Xform xform   = Xform_Get(world, id);

            vec3s min = glms_vec3_sub(xform.pos, body->dims);
            vec3s max = glms_vec3_add(xform.pos, body->dims);
            int hits  = SpatialGrid_Query(spatial->grid_dynamic, min, max, id_buf);
            for (int j = 0; j < hits; j++)
            {
                int idB = id_buf->ids[j];
                if (id == idB)
                    continue;
                ScBody3 *bodyB = Sol_Comp_Get(world, idB, ScBody3);
                if (!bodyB)
                    continue;
                Xform xformB = Xform_Get(world, idB);
                SolContact contact;
                if (shape_pair_test[body->shape][bodyB->shape] &&
                    shape_pair_test[body->shape][bodyB->shape](world, body, xform, bodyB, xformB, &contact))
                {
                    contact.idA = id;
                    contact.idB = idB;
                    solb_push(local_buf->contacts, contact);
                }
            }
        }

#pragma omp for schedule(dynamic)
        for (i = body_count - 1; i >= 0; i--)
        {
            int id        = set->dense[i];
            ScBody3 *body = &set->data[i];
            Xform xform   = Xform_Get(world, id);
            Prof_Begin(&prof_static);
            vec3s min = glms_vec3_sub(xform.pos, body->dims);
            vec3s max = glms_vec3_add(xform.pos, body->dims);
            int hits  = SpatialGrid_Query(spatial->grid_static, min, max, id_buf);
            Prof_EndEz(&prof_static, true, fdt / body_count);
            Prof_Begin(&prof_static_test);
            for (int j = 0; j < hits; j++)
            {
                int idB        = SPATIAL_UNPACK_ID(id_buf->ids[j]);
                int idx        = SPATIAL_UNPACK_IDX(id_buf->ids[j]);
                ScModel *model = Sol_Comp_Get(world, idB, ScModel);
                SolContact contact;
                if (shape_tri_test[body->shape] &&
                    shape_tri_test[body->shape](world, id, idB, &spatial->tris_static[idx], &contact))
                {
                    contact.idA = id;
                    contact.idB = idB;
                    solb_push(local_buf->contacts, contact);
                }
            }
            Prof_EndEz(&prof_static_test, true, fdt / body_count);
        }

#pragma omp critical
        {
            solb_push_array(spatial->contacts, local_buf->contacts, solb_count(local_buf->contacts));
        }
    }

#pragma omp for schedule(dynamic)
    for (i = 0; i < solb_count(spatial->contacts); i++)
    {
        Resolve_Contact(world, spatial->contacts[i]);
    }

    for (i = body_count; i-- > 0;)
    {
        int id        = set->dense[i];
        ScBody3 *body = &set->data[i];
        Xform xform   = Xform_Get(world, id);

        if (xform.pos.y <= -15.0f)
        {
            world->xform.pos[id] = (vec3s){0, 5.0f, 0};
            body->vel            = (vec3s){0, 5.0f, 0};
        }
    }
    Prof_EndEz(&prof_body3, true, fdt);
}

vec3s Sol_Body3_GetGround(World *world, int id)
{
    return (vec3s){0};
}
vec3s Sol_Body3_GetVel(World *world, int id)
{
    return (vec3s){0};
}
vec3s Sol_Body3_GetDir(World *world, int id)
{
    if (Sol_Comp_Has(world, id, ScBody3))
        return glms_vec3_normalize(Sol_Comp_Get(world, id, ScBody3)->vel);
    else
        return (vec3s){0};
}
float Sol_Body3_GetSpeed(World *world, int id)
{
    float speed = 1.0f;
    if (Sol_Comp_Has(world, id, ScBody3))
    {
        speed = glms_vec3_norm(Sol_Comp_Get(world, id, ScBody3)->vel);
    }
    return speed;
}
vec3s Sol_Body3_GetHead(World *world, int id)
{
    vec3s pos      = world->xform.draw_pos[id];
    ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
    if (body3)
        pos.y += body3->dims.y * 0.75f;
    return pos;
}

bool Sol_Raycast1D(World *world, SolRay ray, SolRayResult *result, float time)
{
    bool hit = Sol_Raycast1(world, ray, result);

    SolLine *line = Sol_Debug_NewLine(world, time);
    line->a       = ray.start;
    if (hit && result)
        line->b = Sol_AddScaledDir(ray.start, ray.dir, result->t);
    else
        line->b = Sol_AddScaledDir(ray.start, ray.dir, ray.dist);
    line->aColor = VEC4_RED;
    line->bColor = VEC4_RED;

    return hit;
}

int Sol_RaycastD(World *world, SolRay ray, SolRayResult *results, int max, float time)
{
    int hits = Sol_Raycast(world, ray, results, max);

    SolLine *line = Sol_Debug_NewLine(world, time);
    line->a       = ray.start;
    if (hits > 0)
        line->b = Sol_AddScaledDir(ray.start, ray.dir, results[0].t);
    else
        line->b = Sol_AddScaledDir(ray.start, ray.dir, ray.dist);
    line->aColor = VEC4_RED;
    line->bColor = VEC4_RED;

    return hits;
}

int Sol_SpherecastD(World *world, SolRay ray, SolRayResult *results, int max, float time)
{
    int hits = Sol_Spherecast(world, ray, results, max);

    *Sol_Debug_NewSphere(world, time) = (SolSphere){
        .pos    = ray.start,
        .radius = ray.radius,
        .color  = VEC4_RED,
    };
    *Sol_Debug_NewSphere(world, time) = (SolSphere){
        .pos    = Sol_AddScaledDir(ray.start, ray.dir, ray.dist),
        .radius = ray.radius,
        .color  = VEC4_GREEN,
    };

    return hits;
}

int Sol_SphereOverlapD(World *world, SolRay ray, SolRayResult *out_hits, int max_hits, float time)
{
    int hits                          = Sol_SphereOverlap(world, ray, out_hits, max_hits);
    *Sol_Debug_NewSphere(world, time) = (SolSphere){
        .pos    = ray.start,
        .radius = ray.radius,
        .color  = VEC4_GREEN,
    };
    return hits;
}

bool Sol_Raycast1(World *world, SolRay ray, SolRayResult *outResult)
{
    SparseSet_ScBody3 *set_body = Sol_Comp_Set(world, ScBody3);
    // SlSpatial *spatial          = world->singles[SINGLE_SPATIAL];
    SlSpatial *spatial        = Sol_Comp_Get(world, 0, SlSpatial);
    SpatialGrid *grid_dynamic = spatial->grid_dynamic;
    SpatialGrid *grid_static  = spatial->grid_static;

    outResult->hit = false;
    outResult->t   = ray.dist; // shrinks as closer hits are found; also our search limit

    // --- Dynamic bodies ---
    {
        GridDDA dda     = GridDDA_Init(grid_dynamic, ray.start, ray.dir);
        float current_t = 0.0f;

        while (GridDDA_InBounds(&dda, grid_dynamic) && current_t <= outResult->t)
        {
            uint32_t cell_idx = SpatialGrid_GetCellIdx(grid_dynamic, dda.cell.x, dda.cell.y, dda.cell.z);
            uint32_t start    = grid_dynamic->cell_offsets[cell_idx];
            uint32_t end      = grid_dynamic->cell_offsets[cell_idx + 1];

            // Fully scan this cell before deciding whether to stop —
            // two candidates in the SAME cell can be at different distances.
            for (uint32_t i = start; i < end; i++)
            {
                int id = grid_dynamic->ids[i];
                if (id == ray.ignoreEnt)
                    continue;

                ScBody3 *body = &set_body->data[set_body->sparse[id]];
                if (!body)
                    continue;

                // adjust to your actual mask field/semantics
                if (ray.mask != 0 && (body->mask >> 16 & ray.mask) == 0)
                    continue;

                Xform xform = Xform_Get(world, id);

                SolRayResult local = {0};
                switch (body->shape)
                {
                case SHAPE3_CAP: {
                    vec3s top    = xform.pos;
                    vec3s bottom = xform.pos;
                    top.y += body->dims.y;
                    bottom.y -= body->dims.y;
                    float radius = body->dims.x;

                    float t    = outResult->t;
                    vec3s norm = {0};
                    if (Ray_Intersect_Capsule(ray.start, ray.dir, t, top, bottom, radius, &t, &norm) &&
                        t < outResult->t)
                    {
                        outResult->hit   = true;
                        outResult->t     = t;
                        outResult->norm  = norm;
                        outResult->entId = id;
                    }
                    break;
                }
                }
            }

            current_t = GridDDA_Step(&dda);

            // Nothing in any farther cell can be closer than current_t
            // (we've already fully entered it), so if our best hit is
            // already closer than that, we can stop right here.
            if (outResult->hit && outResult->t <= current_t)
                break;
        }
    }

    // --- Static geometry (triangles) ---
    {
        GridDDA dda     = GridDDA_Init(grid_static, ray.start, ray.dir);
        float current_t = 0.0f;

        while (GridDDA_InBounds(&dda, grid_static) && current_t <= outResult->t)
        {
            uint32_t cell_idx = SpatialGrid_GetCellIdx(grid_static, dda.cell.x, dda.cell.y, dda.cell.z);
            uint32_t start    = grid_static->cell_offsets[cell_idx];
            uint32_t end      = grid_static->cell_offsets[cell_idx + 1];

            for (uint32_t i = start; i < end; i++)
            {
                uint32_t packed = grid_static->ids[i];
                int entId       = SPATIAL_UNPACK_ID(packed);
                int triIdx      = SPATIAL_UNPACK_IDX(packed);

                if (entId == ray.ignoreEnt)
                    continue;

                SolTri *tri = &spatial->tris_static[triIdx];

                float t    = outResult->t;
                vec3s norm = {0};
                if (Ray_Intersect_Tri(ray.start, ray.dir, t, tri, &t, &norm) && t < outResult->t)
                {
                    outResult->hit   = true;
                    outResult->t     = t;
                    outResult->norm  = norm;
                    outResult->entId = entId;
                }
            }

            current_t = GridDDA_Step(&dda);

            if (outResult->hit && outResult->t <= current_t)
                break;
        }
    }

    return outResult->hit;
}

int Sol_Raycast(World *world, SolRay ray, SolRayResult *out_hits, int max_hits)
{
    SparseSet_ScBody3 *set_body = Sol_Comp_Set(world, ScBody3);
    // SlSpatial *spatial          = world->singles[SINGLE_SPATIAL];
    SlSpatial *spatial        = Sol_Comp_Get(world, 0, SlSpatial);
    SpatialGrid *grid_dynamic = spatial->grid_dynamic;
    SpatialGrid *grid_static  = spatial->grid_static;

    int count = 0;

    static _Thread_local HashSlot hash_slot[QUERY_HASH_SIZE];
    static _Thread_local uint32_t local_gen = 0;

    uint32_t this_gen = ++local_gen; // one stamp value covers both tables this call

    // --- Dynamic bodies ---
    {
        GridDDA dda     = GridDDA_Init(grid_dynamic, ray.start, ray.dir);
        float current_t = 0.0f;

        while (GridDDA_InBounds(&dda, grid_dynamic) && current_t <= ray.dist && count < max_hits)
        {
            uint32_t cell_idx = SpatialGrid_GetCellIdx(grid_dynamic, dda.cell.x, dda.cell.y, dda.cell.z);
            uint32_t start    = grid_dynamic->cell_offsets[cell_idx];
            uint32_t end      = grid_dynamic->cell_offsets[cell_idx + 1];

            for (uint32_t i = start; i < end && count < max_hits; i++)
            {
                int id = grid_dynamic->ids[i];
                if (id == ray.ignoreEnt)
                    continue;

                if (!SpatialGrid_HashInsert(hash_slot, this_gen, id))
                    continue;

                ScBody3 *body = &set_body->data[set_body->sparse[id]];
                if (!body)
                    continue;
                if (ray.mask != 0 && ((body->mask >> 16) & ray.mask) == 0)
                    continue;

                Xform xform = Xform_Get(world, id);

                switch (body->shape)
                {
                case SHAPE3_CAP: {
                    vec3s top    = xform.pos;
                    vec3s bottom = xform.pos;
                    top.y += body->dims.y;
                    bottom.y -= body->dims.y;
                    float radius = body->dims.x;

                    float t;
                    vec3s norm;
                    if (Ray_Intersect_Capsule(ray.start, ray.dir, ray.dist, top, bottom, radius, &t, &norm))
                    {
                        out_hits[count].hit   = true;
                        out_hits[count].t     = t;
                        out_hits[count].norm  = norm;
                        out_hits[count].entId = id;
                        count++;
                    }
                    break;
                }
                }
            }

            current_t = GridDDA_Step(&dda);
        }
    }

    // --- Static geometry (triangles) ---
    {
        GridDDA dda     = GridDDA_Init(grid_static, ray.start, ray.dir);
        float current_t = 0.0f;

        while (GridDDA_InBounds(&dda, grid_static) && current_t <= ray.dist && count < max_hits)
        {
            uint32_t cell_idx = SpatialGrid_GetCellIdx(grid_static, dda.cell.x, dda.cell.y, dda.cell.z);
            uint32_t start    = grid_static->cell_offsets[cell_idx];
            uint32_t end      = grid_static->cell_offsets[cell_idx + 1];

            for (uint32_t i = start; i < end && count < max_hits; i++)
            {
                uint32_t packed = grid_static->ids[i];
                int entId       = SPATIAL_UNPACK_ID(packed);
                int triIdx      = SPATIAL_UNPACK_IDX(packed);

                if (entId == ray.ignoreEnt)
                    continue;
                // if (ray.mask != 0 && (COLLAYER_WORLD & ray.mask) == 0)
                //     continue;

                if (!SpatialGrid_HashInsert(hash_slot, this_gen, packed))
                    continue;

                SolTri *tri = &spatial->tris_static[triIdx];

                float t;
                vec3s norm;
                if (Ray_Intersect_Tri(ray.start, ray.dir, ray.dist, tri, &t, &norm))
                {
                    out_hits[count].hit   = true;
                    out_hits[count].t     = t;
                    out_hits[count].norm  = norm;
                    out_hits[count].entId = entId;
                    count++;
                }
            }

            current_t = GridDDA_Step(&dda);
        }
    }

    return count;
}

int Sol_Spherecast(World *world, SolRay ray, SolRayResult *results, int max)
{
    float radius                = ray.radius;
    SparseSet_ScBody3 *set_body = Sol_Comp_Set(world, ScBody3);
    // SlSpatial *spatial          = world->singles[SINGLE_SPATIAL];
    SlSpatial *spatial        = Sol_Comp_Get(world, 0, SlSpatial);
    SpatialGrid *grid_dynamic = spatial->grid_dynamic;
    SpatialGrid *grid_static  = spatial->grid_static;

    int count = 0;

    static _Thread_local HashSlot hash_slot[QUERY_HASH_SIZE];
    static _Thread_local uint32_t local_gen = 0;

    uint32_t this_gen = ++local_gen;

    // --- Dynamic bodies ---
    {
        GridDDA dda     = GridDDA_Init(grid_dynamic, ray.start, ray.dir);
        float current_t = 0.0f;

        while (GridDDA_InBounds(&dda, grid_dynamic) && current_t <= ray.dist && count < max)
        {
            uint32_t cell_idx = SpatialGrid_GetCellIdx(grid_dynamic, dda.cell.x, dda.cell.y, dda.cell.z);
            uint32_t start    = grid_dynamic->cell_offsets[cell_idx];
            uint32_t end      = grid_dynamic->cell_offsets[cell_idx + 1];

            for (uint32_t i = start; i < end && count < max; i++)
            {
                int id = grid_dynamic->ids[i];
                if (id == ray.ignoreEnt)
                    continue;

                if (!SpatialGrid_HashInsert(hash_slot, this_gen, id))
                    continue;

                ScBody3 *body = &set_body->data[set_body->sparse[id]];
                if (!body)
                    continue;

                if (ray.mask != 0 && ((body->mask >> 16) & ray.mask) == 0)
                    continue;

                Xform xform = Xform_Get(world, id);

                switch (body->shape)
                {
                case SHAPE3_CAP: {
                    vec3s top    = xform.pos;
                    vec3s bottom = xform.pos;
                    top.y += body->dims.y;
                    bottom.y -= body->dims.y;

                    // Expand capsule radius by sphere radius for Minkowski sum
                    float combined_radius = body->dims.x + radius;

                    float t;
                    vec3s norm;
                    if (Ray_Intersect_Capsule(ray.start, ray.dir, ray.dist, top, bottom, combined_radius, &t, &norm))
                    {
                        results[count].hit   = true;
                        results[count].t     = t;
                        results[count].norm  = norm;
                        results[count].entId = id;
                        count++;
                    }
                    break;
                }
                default:
                    break;
                }
            }

            current_t = GridDDA_Step(&dda);
        }
    }

    // --- Static geometry (triangles) ---
    {
        GridDDA dda     = GridDDA_Init(grid_static, ray.start, ray.dir);
        float current_t = 0.0f;

        while (GridDDA_InBounds(&dda, grid_static) && current_t <= ray.dist && count < max)
        {
            uint32_t cell_idx = SpatialGrid_GetCellIdx(grid_static, dda.cell.x, dda.cell.y, dda.cell.z);
            uint32_t start    = grid_static->cell_offsets[cell_idx];
            uint32_t end      = grid_static->cell_offsets[cell_idx + 1];

            for (uint32_t i = start; i < end && count < max; i++)
            {
                uint32_t packed = grid_static->ids[i];
                int entId       = SPATIAL_UNPACK_ID(packed);
                int triIdx      = SPATIAL_UNPACK_IDX(packed);

                if (entId == ray.ignoreEnt)
                    continue;

                if (!SpatialGrid_HashInsert(hash_slot, this_gen, packed))
                    continue;

                SolTri *tri = &spatial->tris_static[triIdx];

                float t;
                vec3s norm;
                if (Ray_Intersect_Tri_Thick(ray.start, ray.dir, ray.dist, tri, radius, &t, &norm))
                {
                    results[count].hit   = true;
                    results[count].t     = t;
                    results[count].norm  = norm;
                    results[count].entId = entId;
                    count++;
                }
            }

            current_t = GridDDA_Step(&dda);
        }
    }

    return count;
}

int Sol_SphereOverlap(World *world, SolRay ray, SolRayResult *out_hits, int max_hits)
{
    vec3s center  = ray.start;
    float radius  = ray.radius;
    uint16_t mask = ray.mask;
    int ignoreEnt = ray.ignoreEnt;
    // SlSpatial *spatial          = world->singles[SINGLE_SPATIAL];
    SlSpatial *spatial          = Sol_Comp_Get(world, 0, SlSpatial);
    SparseSet_ScBody3 *set_body = Sol_Comp_Set(world, ScBody3);

    vec3s bmin = glms_vec3_subs(center, radius);
    vec3s bmax = glms_vec3_adds(center, radius);

    static _Thread_local IdBuffer id_buf;
    static _Thread_local bool id_buf_ready = false;
    if (!id_buf_ready)
    {
        solb_init(id_buf.ids, 64);
        id_buf_ready = true;
    }

    int count = 0;

    // --- Dynamic bodies ---
    int found = SpatialGrid_Query(spatial->grid_dynamic, bmin, bmax, &id_buf);
    for (int i = 0; i < found && count < max_hits; i++)
    {
        int id = id_buf.ids[i];
        if (id == ignoreEnt)
            continue;

        ScBody3 *body = &set_body->data[set_body->sparse[id]];
        if (!body)
            continue;
        if (mask != 0 && (body->mask & mask) == 0)
            continue;

        Xform xform = Xform_Get(world, id);

        switch (body->shape)
        {
        case SHAPE3_CAP: {
            vec3s top    = xform.pos;
            vec3s bottom = xform.pos;
            top.y += body->dims.y;
            bottom.y -= body->dims.y;
            float capRadius = body->dims.x;

            float dist;
            vec3s norm;
            if (Sphere_Overlap_Capsule(center, radius, top, bottom, capRadius, &dist, &norm))
            {
                out_hits[count].hit   = true;
                out_hits[count].t     = dist; // distance from center to surface, NOT a ray t
                out_hits[count].norm  = norm;
                out_hits[count].entId = id;
                count++;
            }
            break;
        }
        }
    }

    // --- Static geometry ---
    found = SpatialGrid_Query(spatial->grid_static, bmin, bmax, &id_buf);
    for (int i = 0; i < found && count < max_hits; i++)
    {
        uint32_t packed = id_buf.ids[i];
        int entId       = SPATIAL_UNPACK_ID(packed);
        int triIdx      = SPATIAL_UNPACK_IDX(packed);
        if (entId == ignoreEnt)
            continue;

        SolTri *tri   = &spatial->tris_static[triIdx];
        vec3s closest = ClosestPointOnTriangle(center, tri->a, tri->b, tri->c);
        vec3s delta   = glms_vec3_sub(center, closest);
        float distSq  = glms_vec3_dot(delta, delta);

        if (distSq >= radius * radius)
            continue;

        float dist            = sqrtf(distSq);
        out_hits[count].hit   = true;
        out_hits[count].t     = dist;
        out_hits[count].norm  = dist > 0.0001f ? glms_vec3_scale(delta, 1.0f / dist) : tri->normal;
        out_hits[count].entId = entId;
        count++;
    }

    return count;
}