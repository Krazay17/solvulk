#include <cglm/struct.h>
#include <immintrin.h>
#include <omp.h>

#include "sol_core.h"

const vec3s Sol_Gravity = {0.0f, -9.81f, 0.0f};

static void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform);

void Sol_System_Step_Physx_2d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY2 | HAS_XFORM;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompBody *body = &world->bodies[id];
            CompMovement *movement = &world->movements[id];
            CompXform *xform = &world->xforms[id];

            float moveGrav = MOVE_STATE_FORCES[movement->configId][movement->moveState].gravity;
            body->vel.y = moveGrav ? body->vel.y + moveGrav * fdt
                                   : body->vel.y + Sol_Gravity.y * fdt;
            vec3s finalVel = glms_vec3_scale(body->vel, fdt * 20.0f);
            xform->pos = glms_vec3_add(xform->pos, finalVel);

            if (xform->pos.y + body->height >= solState.windowHeight)
            {
                xform->pos.y = solState.windowHeight - body->height;
                body->vel.y = 0;
            }
        }
    }
}

void Sol_System_Step_Physx_3d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY3 | HAS_XFORM | HAS_MODEL;

    // Pass 1: Integration (Apply Gravity and Move)
    int i;
#pragma omp parallel for
    for (i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        vec3s displacement = {0};
        displacement = glms_vec3_add(displacement, glms_vec3_scale(Sol_Gravity, fdt));
        displacement = glms_vec3_add(displacement, glms_vec3_scale(body->force, fdt));
        displacement = glms_vec3_add(displacement, glms_vec3_scale(body->impulse, fdt));
        body->impulse = (vec3s){0};

        body->vel = glms_vec3_add(body->vel, displacement);
        xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, fdt));

        // Floor check
        if (xform->pos.y < 0)
        {
            xform->pos.y = 0;
            body->vel.y = 0;
            body->grounded = fminf(99999.9f, body->grounded + dt);
            body->airtime = 0;
        }
        else
        {
            body->grounded = 0;
            body->airtime = fminf(99999.9f, body->airtime + dt);
        }
    }

    // Pass 2: Collision Resolution
    for (int i = 0; i < world->activeCount; ++i)
    {
        int idA = world->activeEntities[i];
        if ((world->masks[idA] & required) != required)
            continue;

        for (int j = i + 1; j < world->activeCount; ++j)
        {
            int idB = world->activeEntities[j];
            if ((world->masks[idB] & required) != required)
                continue;

            ResolveCollision(&world->bodies[idA], &world->xforms[idA],
                             &world->bodies[idB], &world->xforms[idB]);
        }
    }
}

static void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform)
{
    vec3s delta = {
        .x = aXform->pos.x - bXform->pos.x,
        .y = aXform->pos.y - bXform->pos.y,
        .z = aXform->pos.z - bXform->pos.z,
    };
    float dist_sq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

    float combined_radius = aBody->radius + bBody->radius;
    float combined_radius_sq = combined_radius * combined_radius;

    // no collision
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
    float invMassA = (aBody->type == BODY_DYNAMIC) ? 1.0f / aBody->mass : 0.0f;
    float invMassB = (bBody->type == BODY_DYNAMIC) ? 1.0f / bBody->mass : 0.0f;
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