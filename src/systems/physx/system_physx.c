#include <cglm/struct.h>
#include <immintrin.h>
#include <omp.h>

#include "sol_core.h"

static SpatialHash spatialHash = {0};
static CompModel *worldModel = {0};
static CompXform *worldXform = {0};
static WorldCollider worldCollider = {0};

void Sol_System_Step_Physx_3d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int steps = SOL_PHYS_SUBSTEP;
    double sub_dt = dt / steps;
    int required = HAS_BODY3 | HAS_XFORM;

    int i;
    int count = world->activeCount;
#pragma omp parallel for if (count > 1000) schedule(guided)
    for (i = 0; i < count; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        vec3s displacement = {0};
        displacement = glms_vec3_add(displacement, glms_vec3_scale(SOL_PHYS_GRAV, fdt));
        displacement = glms_vec3_add(displacement, glms_vec3_scale(body->force, fdt));
        displacement = glms_vec3_add(displacement, glms_vec3_scale(body->impulse, fdt));
        body->impulse = (vec3s){0};

        body->vel = glms_vec3_add(body->vel, displacement);
        xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, fdt));
        SimpleFloor(xform, body, dt);
    }

    BuildSpatialHash(world, &spatialHash);
    for (int s = 0; s < steps; s++)
    {
        ResolveCollisionsSpatial(world, &spatialHash);
    }

    //ResolveWorldCollisions(world);

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
                                   : body->vel.y + SOL_PHYS_GRAV.y * fdt;
            vec3s finalVel = glms_vec3_scale(body->vel, fdt * 20.0f);
            xform->pos = glms_vec3_add(xform->pos, finalVel);

            if (xform->pos.y + body->height >= Sol_GetState()->windowHeight)
            {
                xform->pos.y = Sol_GetState()->windowHeight - body->height;
                body->vel.y = 0;
            }
        }
    }
}

void Sol_Component_Init_Body(CompBody *body)
{
    body->invMass = body->type == BODY_DYNAMIC ? 1.0f / body->mass : 0.0f;
}

