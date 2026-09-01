/*
 * File: s_body3.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-28
 *
 */
#include "s_physx.h"
#include "sol/types.h"
#include "world.h"
#include "model.h"
#include "sol_math.h"
#include "profiler.h"
#include "sol_user.h"
#include "sol_core.h"

#include <omp.h>

static int user_contact_count = 0;

static SolProfiler prof1 = {.name = "Physx"};
static SolProfiler prof2 = {.name = "Dynamic"};
static SolProfiler prof3 = {.name = "Static"};

void Physx_Step(World *world, double dt)
{
    float     fdt = (float)dt;
    int       i, j, k, l, m;
    SysPhysx *ws = world->systems[WORLDSYS_PHYSX];

    Build_Tables(world, ws, fdt);
    solb_zero(ws->contacts);

    SparseSet_SolBody3 *set   = Sol_Comp_Set(world, SolBody3);
    int                 count = set->cnt;
    // -------------------------------------------------------------
    // 1. Integration Step (Single Threaded or OpenMP Parallel)
    // -------------------------------------------------------------
    for (i = 0; i < set->cnt; i++)
    {
        int       id    = set->dense[i];
        SolBody3 *body3 = &set->data[i];
        SolXform *xform = Sol_Comp_Get(world, id, SolXform);
        if (!xform || body3->mass == 0.0f)
            continue;

        body3->vel     = glms_vec3_scale(body3->vel, 0.99f);
        vec3s accel    = body3->vel.y < TERMINAL_VELOCITY ? GLMS_VEC3_ZERO : body3->gravity;
        accel          = glms_vec3_add(accel, body3->force);
        accel          = glms_vec3_add(accel, body3->impulse);
        body3->impulse = (vec3s){0};

        body3->vel = glms_vec3_add(body3->vel, glms_vec3_scale(accel, fdt));
        xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body3->vel, fdt));
    }

    // -------------------------------------------------------------
    // 2. Dynamic vs Dynamic Collisions (Thread-Local Scratchpads)
    // -------------------------------------------------------------
    Prof_Begin(&prof2);
#pragma omp parallel if (count > 100)
    {
        ThreadContactBuffer local_buf = {0};

#pragma omp for schedule(static, 64)
        for (j = 0; j < set->cnt; j++)
        {
            int       id    = set->dense[j];
            SolBody3 *body3 = &set->data[j];
            SolXform *xform = Sol_Comp_Get(world, id, SolXform);
            if (!xform || body3->mass == 0.0f)
                continue;

            // Pass local_buf to receive contacts safely without global locking
            Collisions_Dynamic_Bodies_Local(world, id, ws, xform, body3, &local_buf);
        }

        // Merge thread-local contacts into global array once per thread
        if (local_buf.count > 0)
        {
#pragma omp critical
            {
                solb_push_array(ws->contacts, local_buf.contacts, local_buf.count);
            }
        }
    }
    Prof_EndEz(&prof2, true, dt);

    // -------------------------------------------------------------
    // 3. Static Stage Collisions (Thread-Local Scratchpads)
    // -------------------------------------------------------------
    Prof_Begin(&prof3);
#pragma omp parallel if (count > 100)
    {
        ThreadContactBuffer local_buf = {0};

#pragma omp for schedule(static, 64)
        for (k = 0; k < set->cnt; k++)
        {
            int       id    = set->dense[k];
            SolBody3 *body3 = &set->data[k];
            SolXform *xform = Sol_Comp_Get(world, id, SolXform);
            if (!xform || body3->mass == 0.0f)
                continue;

            Collisions_Static_Stage_Local(world, id, ws, xform, body3, fdt, &local_buf);
        }

        if (local_buf.count > 0)
        {
#pragma omp critical
            {
                solb_push_array(ws->contacts, local_buf.contacts, local_buf.count);
            }
        }
    }
    Prof_EndEz(&prof3, true, dt);

    // -------------------------------------------------------------
    // 4. Contact Resolution (Single Threaded)
    // -------------------------------------------------------------
    user_contact_count = 0;
    for (l = 0; l < solb_count(ws->contacts); l++)
    {
        SolContact *contact = &ws->contacts[l];

        SolBody3 *bodyA  = Sol_Comp_Get(world, contact->id, SolBody3);
        SolXform *xformA = Sol_Comp_Get(world, contact->id, SolXform);
        SolBody3 *bodyB  = Sol_Comp_Get(world, contact->idB, SolBody3);
        SolXform *xformB = Sol_Comp_Get(world, contact->idB, SolXform);

        Resolve_Contact(bodyA, xformA, bodyB, xformB, contact);

        if (contact->id == user_session.user_entid)
            user_contact_count++;
    }
    if (user_contact_count != 0)
        Sol_Debug_Add("User Contacts", user_contact_count);

    // -------------------------------------------------------------
    // 5. Out-of-bounds reset loop
    // -------------------------------------------------------------
    for (m = 0; m < set->cnt; m++)
    {
        int       id    = set->dense[m];
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
    solb_init(ws->contacts, MAX_CONTACTS);
    solb_init(ws->worldTris, WORLD_TRI_INIT);
}

void Physx_Deinit(World *world)
{
    SysPhysx *ws = world->systems[WORLDSYS_PHYSX];
    if (ws)
    {
        SpatialTable_Free(&ws->dynamic_table);
        SpatialTable_Free(&ws->dynamic_tri_table);
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
