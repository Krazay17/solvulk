#include "s_body3.h"
#include "model.h"
#include "sol_math.h"
#include "world.h"

bool Collide_Sphere_Sphere(World *world, int idA, int idB, SolContact *contact)
{
    SolXform *xform  = Sol_Comp_Get(world, idA, SolXform);
    SolXform *xformB = Sol_Comp_Get(world, idB, SolXform);
    SolBody3 *body3  = Sol_Comp_Get(world, idA, SolBody3);
    SolBody3 *body3B = Sol_Comp_Get(world, idB, SolBody3);

    vec3s delta     = glms_vec3_sub(xform->pos, xformB->pos);
    float distSq    = glms_vec3_dot(delta, delta);
    float radiusSum = body3->dims.x + body3B->dims.x;

    if (distSq >= (radiusSum * radiusSum) || distSq < 0.0001f)
        return false;

    float distance = sqrtf(distSq);

    contact->normal      = glms_vec3_scale(delta, 1.0f / distance);
    contact->penetration = radiusSum - distance;
    contact->pos         = glms_vec3_add(xformB->pos, glms_vec3_scale(contact->normal, contact->penetration));

    return true;
}

bool Collide_Capsule_Capsule(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}
bool Collide_Sphere_Capsule(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}
bool Collide_Capsule_Sphere(World *world, int idA, int idB, SolContact *hit)
{
    return false;
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
    SolXform *xform  = Sol_Comp_Get(world, idA, SolXform);
    SolBody3 *body3  = Sol_Comp_Get(world, idA, SolBody3);

    vec3s closestP = ClosestPointOnTriangle(xform->pos, tri->a, tri->b, tri->c);
    vec3s delta    = glms_vec3_sub(xform->pos, closestP);
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

bool Collide_Capsule_Model(World *world, int idA, int idB, SolContact *hit)
{
    return false;
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