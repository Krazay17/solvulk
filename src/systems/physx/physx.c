#include <cglm/struct.h>
#include <omp.h>

#include "sol_core.h"

static u32         ents[MAX_ENTS];
static SolProfiler prof_static  = {.name = "Static"};
static SolProfiler prof_dynamic = {.name = "Dynamic"};

static int prof_frame = 0;

void Physx_Init(World *world)
{
    world->spatial = calloc(1, sizeof(WorldPhysx));
    SpatialTable_Init(&world->spatial->staticGroup.table, SPATIAL_STATIC_SIZE, SPATIAL_STATIC_ENTRIES,
                      SPATIAL_STATIC_CELL_SIZE);
    SpatialTable_Init(&world->spatial->dynamicGroup.table, SPATIAL_DYNAMIC_SIZE, SPATIAL_DYNAMIC_ENTRIES,
                      SPATIAL_DYNAMIC_CELL_SIZE);
}

CompBody *Sol_Physx_Add(World *world, int id, CompBody init_body)
{
    CompBody body     = init_body;
    body.height       = body.height ? body.height : 0.5f;
    body.radius       = body.radius ? body.radius : 0.5f;
    body.length       = body.length ? body.length : 0.5f;
    body.invMass      = body.mass > 0 ? 1.0f / body.mass : 0;
    world->bodies[id] = body;

    Spatial_Add(world, id, &world->bodies[id]);

    world->masks[id] |= HAS_BODY3;
    return &world->bodies[id];
}

void Physx_Step(World *world, double dt, double time)
{
    if (Sol_GetState()->fps < 30)
        return;
    float       fdt      = (float)dt;
    int         required = HAS_BODY3 | HAS_XFORM;
    int         i, j, k, l;
    int         count        = 0;
    int         activeCount  = world->activeCount;
    WorldPhysx *ws           = world->spatial;
    PhysxGroup *staticGroup  = &ws->staticGroup;
    PhysxGroup *dynamicGroup = &ws->dynamicGroup;

    Physx_Grid_Static_Rebuild(staticGroup);

    // Filter Entities
    for (i = 0; i < activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        if (world->bodies[id].mass == 0)
            continue;
        ents[count++] = id;
    }

    Prof_Begin(&prof_static);
    // velocity and static collisions
#pragma omp parallel for if (count > 500) schedule(dynamic, 16)
    for (j = 0; j < count; j++)
    {
        int        id    = ents[j];
        CompBody  *body  = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        vec3s accel   = SOL_PHYS_GRAV;
        accel         = glms_vec3_add(accel, body->force);
        accel         = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel     = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));

        SolCollision    col      = {0};
        ResolveShapeTri resolver = shape_tri_resolvers[body->shape];

        SubstepData substep = Substep_Get(body, fdt);
        for (int s = 0; s < substep.substeps; s++)
        {
            xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, substep.sub_dt));
            col        = Collisions_Static_Grid(staticGroup, body, xform, resolver);
        }
        Physx_Ground_Trace(world, body, xform);
    }

    Fill_Dynamic_Table(world, count, ents);

    Prof_Begin(&prof_dynamic);
    // Dynamic collisions
#pragma omp parallel for if (count > 500) schedule(dynamic, 16)
    for (k = 0; k < count; k++)
    {
        int        id    = ents[k];
        CompBody  *body  = &world->bodies[id];
        CompXform *xform = &world->xforms[id];
        Collisions_Dynamic_Hashed(world, id, body, xform);
    }
    Prof_End(&prof_dynamic);

    Prof_End(&prof_static);
    if (prof_frame++ % 200 == 0)
    {
        Prof_Print(&prof_static);
        Prof_Print(&prof_dynamic);
    }
}

SolRayResult Sol_Raycast(World *world, SolRay ray)
{
    SolRayResult result = {0};
    result.dist         = ray.dist;
    WorldPhysx *ws      = world->spatial;

    float dirLen = glms_vec3_norm(ray.dir);
    if (dirLen < FLOATING_EPSILON)
    {
        result.pos = ray.pos;
        return result;
    }
    ray.dir          = glms_vec3_scale(ray.dir, 1.0f / dirLen);
    result.pos       = glms_vec3_add(ray.pos, glms_vec3_scale(ray.dir, ray.dist));
    SolRayResult sub = {0};

    sub = Raycast_Static_Grid(&ws->staticGroup, ray);
    if (sub.hit && sub.dist < result.dist)
        result = sub;

    return result;
}