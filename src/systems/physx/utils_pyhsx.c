#include "sol_core.h"
#include "cglm/struct.h"

void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform)
{
    vec3s delta = glms_vec3_sub(aXform->pos, bXform->pos);
    float combined_radius = aBody->radius + bBody->radius;
    float dist_sq = glms_vec3_dot(delta, delta);

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

SolCollision ResolveSphereTriangle(CompBody *body, CompXform *xform, CollisionTri *tri)
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

    // float side = glms_vec3_dot(glms_vec3_sub(*localPos, tri->a), tri->normal);
    // if (side < 0)
    // {
    //     // Behind the face — push out along face normal (through to front)
    //     normal = tri->normal;
    //     float penetration = body->radius + dist;
    //     *localPos = glms_vec3_add(*localPos, glms_vec3_scale(normal, penetration));
    // }
    // else
    // {
    //     // In front — normal behavior
    // }

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

SolCollision ResolveCapsuleTriangle(CompBody *body, CompXform *xform, CollisionTri *tri)
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