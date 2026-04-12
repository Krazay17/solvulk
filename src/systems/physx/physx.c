#include <cglm/struct.h>
#include <immintrin.h>
#include <omp.h>

#include "sol_core.h"

#define CELL_SIZE 2.0f
#define MAX_ENTITIES_PER_CELL 32
#define TABLE_SIZE 8192
#define GRID_WIDTH 1024
#define GRID_DEPTH 1024
#define GRID_HEIGHT 128

typedef struct
{
    int entityIds[MAX_ENTITIES_PER_CELL];
    int count;
} SpatialCell;

typedef struct
{
    SpatialCell cells[TABLE_SIZE];
} SpatialHash;

SpatialHash spatialHash = {0};
const vec3s Sol_Gravity = {0.0f, -9.81f, 0.0f};
static int GetHash(vec3s pos);
static void BuildSpatialHash(World *world, SpatialHash *grid);
static void ResolveCollisionsSpatial(World *world, SpatialHash *grid);
static void ResolveCollision(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform);

void Sol_System_Step_Physx_3d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY3 | HAS_XFORM | HAS_MODEL;

    int i;
//#pragma omp parallel for
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

    BuildSpatialHash(world, &spatialHash);
    int steps = 4;
    double sub_dt = dt / steps;
    for (int s = 0; s < steps; s++)
    {
        ResolveCollisionsSpatial(world, &spatialHash);
    }
    
}

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

static int GetHash(vec3s pos)
{
    // Quantize position into integer grid coordinates
    int ix = (int)floorf(pos.x / CELL_SIZE);
    int iy = (int)floorf(pos.y / CELL_SIZE);
    int iz = (int)floorf(pos.z / CELL_SIZE);

    // Spatial Hash formula (using primes)
    unsigned int hash = ((unsigned int)ix * 73856093) ^
                        ((unsigned int)iy * 19349663) ^
                        ((unsigned int)iz * 83492791);

    return hash % TABLE_SIZE;
}

static void BuildSpatialHash(World *world, SpatialHash *grid)
{
    memset(grid->cells, 0, sizeof(SpatialCell) * TABLE_SIZE);

    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & HAS_BODY3))
            continue;

        int hash = GetHash(world->xforms[id].pos);
        SpatialCell *cell = &grid->cells[hash];

        // atomic_fetch_add here
        if (cell->count < MAX_ENTITIES_PER_CELL)
        {
            cell->entityIds[cell->count++] = id;
        }
    }
}

static void ResolveCollisionsSpatial(World *world, SpatialHash *grid)
{
    // Use OMP on the entity loop.
    // Since we only ResolveCollision if idA < idB, it is mostly thread-safe.
    int i;
#pragma omp parallel for schedule(guided)
    for (i = 0; i < world->activeCount; i++)
    {
        int idA = world->activeEntities[i];
        if (!(world->masks[idA] & HAS_BODY3))
            continue;

        vec3s posA = world->xforms[idA].pos;
        int ix = (int)floorf(posA.x / CELL_SIZE);
        int iy = (int)floorf(posA.y / CELL_SIZE);
        int iz = (int)floorf(posA.z / CELL_SIZE);

        for (int ox = -1; ox <= 1; ox++)
        {
            for (int oy = -1; oy <= 1; oy++)
            {
                for (int oz = -1; oz <= 1; oz++)
                {
                    unsigned int hash = ((unsigned int)(ix + ox) * 73856093) ^
                                        ((unsigned int)(iy + oy) * 19349663) ^
                                        ((unsigned int)(iz + oz) * 83492791);

                    SpatialCell *cell = &grid->cells[hash % TABLE_SIZE];

                    // PRE-FETCH count to avoid cache misses in the loop
                    int cellCount = cell->count;
                    for (int n = 0; n < cellCount; n++)
                    {
                        int idB = cell->entityIds[n];

                        // This is the most important line for performance:
                        // It ensures we only check the pair once AND avoids self-collision.
                        if (idA >= idB)
                            continue;

                        ResolveCollision(&world->bodies[idA], &world->xforms[idA],
                                         &world->bodies[idB], &world->xforms[idB]);
                    }
                }
            }
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

void Sol_Component_Init_Body(CompBody *body)
{
    body->invMass = body->type == BODY_DYNAMIC ? 1.0f / body->mass : 0.0f;
}