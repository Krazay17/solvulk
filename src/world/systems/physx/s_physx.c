/*
 * File: s_body3.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-28
 *
 */
#include "s_physx.h"
#include "world.h"
#include "model.h"
#include "sol_math.h"
#include "profiler.h"

#include <omp.h>

static SolProfiler prof1 = {.name = "Physx"};
static SolProfiler prof2 = {.name = "Dynamic"};
static SolProfiler prof3 = {.name = "Static"};

void Physx_Step(World *world, double dt)
{
    float     fdt = (float)dt;
    SysPhysx *ws  = world->systems[WORLDSYS_PHYSX];
    Build_Tables(world, ws, fdt);
    ws->contacts.contact_cnt = 0;
    Prof_Begin(&prof1);

    SparseSet_SolBody3 *set = Sol_Comp_Set(world, SolBody3);
    for (int i = 0; i < set->cnt; i++)
    {
        int       id    = set->dense[i];
        SolBody3 *body3 = &set->data[i];
        SolXform *xform = Sol_Comp_Get(world, id, SolXform);
        if (!xform || body3->mass == 0.0f)
            continue;

        body3->vel     = vecSca(body3->vel, 0.99f);
        vec3s accel    = body3->vel.y < TERMINAL_VELOCITY ? GLMS_VEC3_ZERO : body3->gravity;
        accel          = glms_vec3_add(accel, body3->force);
        accel          = glms_vec3_add(accel, body3->impulse);
        body3->impulse = (vec3s){0};
        body3->vel     = glms_vec3_add(body3->vel, glms_vec3_scale(accel, fdt));
        xform->pos     = glms_vec3_add(xform->pos, glms_vec3_scale(body3->vel, fdt));

        Prof_Begin(&prof2);
        Collisions_Dynamic_Bodies(world, id, ws, xform, body3);
        Prof_EndEz(&prof2, true, dt / (double)set->cnt);
        Collisions_Dynamic_Tris(world, id, ws, xform, body3);
        Prof_Begin(&prof3);
        Collisions_Static_Stage(world, id, ws, xform, body3, fdt);
        Prof_EndEz(&prof3, true, dt / (double)set->cnt);
    }
    Prof_EndEz(&prof1, true, dt);

    for (int i = 0; i < ws->contacts.contact_cnt; i++)
    {
        SolContact *contact = &ws->contacts.contact[i];

        SolBody3 *bodyA  = Sol_Comp_Get(world, contact->id, SolBody3);
        SolXform *xformA = Sol_Comp_Get(world, contact->id, SolXform);
        SolBody3 *bodyB  = Sol_Comp_Get(world, contact->idB, SolBody3);
        SolXform *xformB = Sol_Comp_Get(world, contact->idB, SolXform);

        Resolve_Contact(bodyA, xformA, bodyB, xformB, contact);
    }

    for (int i = 0; i < set->cnt; i++)
    {
        int       id    = set->dense[i];
        SolXform *xform = Sol_Comp_Get(world, id, SolXform);
        if (xform && xform->pos.y < -20.0f)
        {
            xform->pos = (vec3s){0, 10, 0};
        }
    }
}

void Physx_Init(World *world)
{
    SysPhysx *ws                   = malloc(sizeof(SysPhysx));
    world->systems[WORLDSYS_PHYSX] = ws;
    SpatialTable_Init(&ws->dynamic_table, SPATIAL_SIZE, SPATIAL_CAP, SPATIAL_CELL_SIZE);
    SpatialTable_Init(&ws->dynamic_tri_table, SPATIAL_TRI_SIZE, SPATIAL_TRI_CAP, SPATIAL_TRI_CELL_SIZE);
    SpatialGrid_Init(&ws->static_tri_grid, SPATIAL_STATIC_TRI_CELL_SIZE);
}

void Physx_Deinit(World *world)
{
    SysPhysx *ws = world->systems[WORLDSYS_PHYSX];
    if (ws)
    {
        SpatialTable_Free(&ws->dynamic_table);
        SpatialTable_Free(&ws->dynamic_tri_table);
        SpatialTable_Free(&ws->static_tri_table);
        SpatialGrid_Destroy(&ws->static_tri_grid);
        free(ws);
        world->systems[WORLDSYS_PHYSX] = NULL;
    }
}

bool Sol_Physx_DoesCollide(SolBody3 *body, SolBody3 *other_body)
{
    return (body->group << 16) & (other_body->group);
}

vec3s Sol_Physx_GetGround(World *world, int id)
{
    return GLMS_VEC3_ZERO;
}

int Sol_Physx_Raycast(World *world, SolRay ray, SolRayResult *result, int max)
{
    return 0;
}
