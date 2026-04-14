#include "sol_core.h"
#include "cglm/struct.h"

void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform)
{
    vec3s delta = {
        .x = aXform->pos.x - bXform->pos.x,
        .y = aXform->pos.y - bXform->pos.y,
        .z = aXform->pos.z - bXform->pos.z,
    };

    float combined_radius = aBody->radius + bBody->radius;

    float dx = aXform->pos.x - bXform->pos.x;
    if (fabsf(dx) > combined_radius)
        return;

    float dy = aXform->pos.y - bXform->pos.y;
    if (fabsf(dy) > combined_radius)
        return;

    float dz = aXform->pos.z - bXform->pos.z;
    if (fabsf(dz) > combined_radius)
        return;

    float combined_radius_sq = combined_radius * combined_radius;
    float dist_sq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    if (dist_sq >= combined_radius_sq || dist_sq <= 0.0f)
        return;

    float distance = sqrt(dist_sq);
    float penetration = combined_radius - distance;
    float invDist = 1.0f / distance;
    vec3s normal = {
        .x = delta.x * invDist,
        .y = delta.y * invDist,
        .z = delta.z * invDist,
    };
    float invMassA = aBody->invMass;
    float invMassB = bBody->invMass;
    float totalInvMass = invMassA + invMassB;

    if (totalInvMass <= 0.0f)
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

    vec3s closestP = ClosestPointOnTriangle(*localPos, tri->a, tri->b, tri->c);
    vec3s delta = glms_vec3_sub(*localPos, closestP);
    float distSq = glms_vec3_dot(delta, delta);

    float radiusSq = sphereBody->radius * sphereBody->radius;
    if (distSq >= radiusSq)
        return result;

    float dist = sqrtf(distSq);
    vec3s normal = dist < 0.0001f
        ? tri->normal
        : glms_vec3_scale(delta, 1.0f / dist);

    float penetration = sphereBody->radius - dist;
    *localPos = glms_vec3_add(*localPos, glms_vec3_scale(normal, penetration));

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