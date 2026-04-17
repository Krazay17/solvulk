#include "sol_core.h"
#include "cglm/struct.h"

void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform)
{
    vec3s delta = glms_vec3_sub(aXform->pos, bXform->pos);
    float combined_radius = aBody->radius + bBody->radius;
    float dist_sq = glms_vec3_dot(delta, delta);

    // FIX 1: Increased epsilon to prevent floating point explosions
    if (dist_sq >= (combined_radius * combined_radius) || dist_sq < 0.0001f)
        return;

    float distance = sqrt(dist_sq);
    float penetration = combined_radius - distance;
    vec3s normal = glms_vec3_scale(delta, 1.0f / distance);

    float invMassA = aBody->invMass;
    float invMassB = bBody->invMass;
    float totalInvMass = invMassA + invMassB;

    if (totalInvMass <= 0.000001f)
        return;

    float slack = 0.01f;
    float correctionMagnitude = fmaxf(penetration - slack, 0.0f) / totalInvMass;
    vec3s correctionVector = glms_vec3_scale(normal, correctionMagnitude);

    aXform->pos = glms_vec3_add(aXform->pos, glms_vec3_scale(correctionVector, invMassA));
    bXform->pos = glms_vec3_sub(bXform->pos, glms_vec3_scale(correctionVector, invMassB));

    vec3s relativeVel = glms_vec3_sub(aBody->vel, bBody->vel);
    float velAlongNormal = glms_vec3_dot(relativeVel, normal);

    if (velAlongNormal > 0)
        return;

    float e = fminf(aBody->restitution, bBody->restitution);
    float j = -(1.0f + e) * velAlongNormal;
    j /= totalInvMass;

    vec3s impulse = glms_vec3_scale(normal, j);

    aBody->vel = glms_vec3_add(aBody->vel, glms_vec3_scale(impulse, invMassA));
    bBody->vel = glms_vec3_sub(bBody->vel, glms_vec3_scale(impulse, invMassB));
}

SolCollision ResolveSphereTriangle(CompBody *sphereBody, vec3s *localPos, CollisionTri *tri)
{
    SolCollision result = {0};

    // Bounding sphere early-out
    float dx = localPos->x - tri->center.x;
    float dy = localPos->y - tri->center.y;
    float dz = localPos->z - tri->center.z;
    float maxDist = sphereBody->radius + tri->boundRadius;
    if (dx * dx + dy * dy + dz * dz > maxDist * maxDist)
        return result;

    vec3s closestP = ClosestPointOnTriangle(*localPos, tri->a, tri->b, tri->c);
    vec3s delta = glms_vec3_sub(*localPos, closestP);
    float distSq = glms_vec3_dot(delta, delta);

    float radiusSq = sphereBody->radius * sphereBody->radius;
    if (distSq >= radiusSq)
        return result;

    float dist = sqrtf(distSq);

    
    float side = glms_vec3_dot(glms_vec3_sub(*localPos, tri->a), tri->normal);
    vec3s normal;
    if (side < 0)
    {
        // Behind the face — push out along face normal (through to front)
        normal = tri->normal;
        float penetration = sphereBody->radius + dist;
        *localPos = glms_vec3_add(*localPos, glms_vec3_scale(normal, penetration));
    }
    else
    {
        // In front — normal behavior
        normal = dist > 0.0001f
            ? glms_vec3_scale(delta, 1.0f / dist)
            : tri->normal;
        float penetration = sphereBody->radius - dist;
        *localPos = glms_vec3_add(*localPos, glms_vec3_scale(normal, penetration));
    }

    float velAlongNormal = glms_vec3_dot(sphereBody->vel, normal);
    if (velAlongNormal < 0)
    {
        sphereBody->vel = glms_vec3_sub(
            sphereBody->vel,
            glms_vec3_scale(normal, velAlongNormal * sphereBody->restitution));
    }

    result.didCollide = true;
    result.pos = closestP;
    result.normal = normal;
    result.vel = sphereBody->vel;

    return result;
}

void SimpleFloor(CompXform *xform, CompBody *body, float dt)
{

    float pen = 0.0f - (xform->pos.y - body->height / 2.0f);
    if (pen > 0)
    {
        body->grounded = fminf(999.9f, body->grounded + dt);
        body->airtime = 0;

        xform->pos.y += pen;
        body->vel.y = 0;
    }
    else
    {
        body->grounded = 0;
        body->airtime = fminf(999.9f, body->airtime + dt);
    }
}

// TriAABB Sol_GetTriAABB(vec3s a, vec3s b, vec3s c)
// {
//     return (TriAABB){
//         .minX = fminf(a.x, fminf(b.x, c.x)),
//         .maxX = fmaxf(a.x, fmaxf(b.x, c.x)),
//         .minY = fminf(a.y, fminf(b.y, c.y)),
//         .maxY = fmaxf(a.y, fmaxf(b.y, c.y)),
//         .minZ = fminf(a.z, fminf(b.z, c.z)),
//         .maxZ = fmaxf(a.z, fmaxf(b.z, c.z)),
//     };
// }

// vec3s ClosestPointOnTriangle(vec3s p, vec3s a, vec3s b, vec3s c)
// {
//     vec3s ab = glms_vec3_sub(b, a);
//     vec3s ac = glms_vec3_sub(c, a);
//     vec3s ap = glms_vec3_sub(p, a);

//     // Check if P in vertex region outside A
//     float d1 = glms_vec3_dot(ab, ap);
//     float d2 = glms_vec3_dot(ac, ap);
//     if (d1 <= 0.0f && d2 <= 0.0f)
//         return a;

//     // Check if P in vertex region outside B
//     vec3s bp = glms_vec3_sub(p, b);
//     float d3 = glms_vec3_dot(ab, bp);
//     float d4 = glms_vec3_dot(ac, bp);
//     if (d3 >= 0.0f && d4 <= d3)
//         return b;

//     // Check if P in edge region of AB, if so return projection of P onto AB
//     float vc = d1 * d4 - d3 * d2;
//     if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
//     {
//         float denom = d1 - d3;
//         // Guard against division by zero
//         float v = (denom <= 0.0f) ? 0.0f : d1 / denom;
//         return glms_vec3_add(a, glms_vec3_scale(ab, v));
//     }

//     // Check if P in vertex region outside C
//     vec3s cp = glms_vec3_sub(p, c);
//     float d5 = glms_vec3_dot(ab, cp);
//     float d6 = glms_vec3_dot(ac, cp);
//     if (d6 >= 0.0f && d5 <= d6)
//         return c;

//     // Check if P in edge region of AC, if so return projection of P onto AC
//     float vb = d5 * d2 - d1 * d6;
//     if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
//     {
//         float denom = d2 - d6;
//         // Guard against division by zero
//         float w = (denom <= 0.0f) ? 0.0f : d2 / denom;
//         return glms_vec3_add(a, glms_vec3_scale(ac, w));
//     }

//     // Check if P in edge region of BC, if so return projection of P onto BC
//     float va = d3 * d6 - d5 * d4;
//     if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
//     {
//         float denom = (d4 - d3) + (d5 - d6);
//         // Guard against division by zero
//         float w = (denom <= 0.0f) ? 0.0f : (d4 - d3) / denom;
//         return glms_vec3_add(b, glms_vec3_scale(glms_vec3_sub(c, b), w));
//     }

//     // P inside face region. Compute Q through barycentric coordinates (u,v,w)
//     float d_sum = va + vb + vc;

//     // If we reach here and d_sum is 0, the triangle is degenerate (a line or point).
//     // Returning 'a' prevents the NaN explosion.
//     if (d_sum <= 0.0f)
//         return a;

//     float denom = 1.0f / d_sum;
//     float v = vb * denom;
//     float w = vc * denom;

//     // Result = a + v*ab + w*ac
//     return glms_vec3_add(a, glms_vec3_add(glms_vec3_scale(ab, v), glms_vec3_scale(ac, w)));
// }

vec3s ClosestPointOnTriangle(vec3s p, vec3s a, vec3s b, vec3s c)
{
    vec3s ab = glms_vec3_sub(b, a);
    vec3s ac = glms_vec3_sub(c, a);
    vec3s ap = glms_vec3_sub(p, a);

    float d1 = glms_vec3_dot(ab, ap);
    float d2 = glms_vec3_dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
        return a;

    vec3s bp = glms_vec3_sub(p, b);
    float d3 = glms_vec3_dot(ab, bp);
    float d4 = glms_vec3_dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
        return b;

    vec3s cp = glms_vec3_sub(p, c);
    float d5 = glms_vec3_dot(ab, cp);
    float d6 = glms_vec3_dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
        return c;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float v = d1 / (d1 - d3);
        return glms_vec3_add(a, glms_vec3_scale(ab, v));
    }

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        float w = d2 / (d2 - d6);
        return glms_vec3_add(a, glms_vec3_scale(ac, w));
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return glms_vec3_add(b, glms_vec3_scale(glms_vec3_sub(c, b), w));
    }

    float denom = va + vb + vc;
    if (denom <= 1e-8f)
        return a;

    float inv = 1.0f / denom;
    float v = vb * inv;
    float w = vc * inv;
    return glms_vec3_add(a, glms_vec3_add(glms_vec3_scale(ab, v), glms_vec3_scale(ac, w)));
}