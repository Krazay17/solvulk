/*
 * File: s_body3.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-28
 *
 */
#include "s_body3.h"
#include "world.h"
#include "model.h"
#include "sol_math.h"
#include "spatial.h"
#include "profiler.h"

#include <omp.h>

#define SPATIAL_SIZE 0x10000
#define SPATIAL_CAP 0xFFFFFU
#define SPATIAL_CELL_SIZE 2.0f

#define SPATIAL_TRI_SIZE 0x10000
#define SPATIAL_TRI_CAP 0xFFFFFU
#define SPATIAL_TRI_CELL_SIZE 4.0f

// #define SPATIAL_STATIC_TRI_SIZE 0xFFFFU
// #define SPATIAL_STATIC_TRI_CAP 0xFFFFFFFU
// #define SPATIAL_STATIC_TRI_CELL_SIZE 8.0f

#define MAX_COLLISION_TRIS (1 << 18)
#define MAX_CONTACTS (1 << 16)
#define TERMINAL_VELOCITY -100.0f

#define MAKE_TRI_VALUE(entID, triIdx) ((((u32)(entID) & 0xFFFF) << 16) | ((u32)(triIdx) & 0xFFFF))
#define GET_TRI_ENTITY(val) (((val) >> 16) & 0xFFFF)
#define GET_TRI_INDEX(val) ((val) & 0xFFFF)

typedef struct
{
    SolContact contact[MAX_CONTACTS];
    int        contact_cnt;

} Body3Contacts;
typedef struct
{
    SpatialTable  dynamic_table;
    SpatialTable  dynamic_tri_table;
    Body3Contacts contacts;
} Body3Sys;

typedef struct
{
    u8    substeps;
    float sub_dt;
} SubstepData;

typedef bool (*ShapePairTest)(World *world, int idA, int idB, SolContact *contact);
const ShapePairTest shape_pair_test[SHAPE3_CNT][SHAPE3_CNT] = {
    [SHAPE3_SPH][SHAPE3_SPH] = Collide_Sphere_Sphere,
    [SHAPE3_CAP][SHAPE3_CAP] = Collide_Capsule_Capsule,
    [SHAPE3_CAP][SHAPE3_SPH] = Collide_Capsule_Sphere,
    [SHAPE3_SPH][SHAPE3_CAP] = Collide_Sphere_Capsule,
};
typedef bool (*ShapeTriTest)(World *world, int idA, int idB, const SolTri *tri, SolContact *contact);
const ShapeTriTest shape_tri_test[SHAPE3_CNT] = {
    [SHAPE3_SPH] = Collide_Sphere_Tri,
};

SolProfiler body_profiler = {.name = "Body"};

void Resolve_Dynamic_Pair(SolBody3 *aBody, SolXform *aXform, SolBody3 *bBody, SolXform *bXform, SolContact *contact)
{
    float totalInvMass = aBody->invMass + bBody->invMass;
    if (totalInvMass <= 0.0f)
        return;

    // --- 1. Positional Correction (The "Push Out") ---
    // We use "slack" (also called a "slop") to prevent jittering when objects barely touch.
    float slack   = 0.05f;
    float percent = 0.9f;

    float corrMag    = (fmaxf(contact->penetration - slack, 0.0f) / totalInvMass) * percent;
    vec3s correction = glms_vec3_scale(contact->normal, corrMag);

    aXform->pos = glms_vec3_add(aXform->pos, glms_vec3_scale(correction, aBody->invMass));
    bXform->pos = glms_vec3_sub(bXform->pos, glms_vec3_scale(correction, bBody->invMass));

    // --- 2. Velocity Resolution (The "Bounce") ---
    vec3s relativeVel    = glms_vec3_sub(aBody->vel, bBody->vel);
    float velAlongNormal = glms_vec3_dot(relativeVel, contact->normal);

    // If objects are already moving apart, don't apply an impulse
    if (velAlongNormal > 0)
        return;

    // Use the lower restitution of the two objects
    float e = fminf(aBody->restitution, bBody->restitution);

    // Standard Impulse Formula: j = -(1+e)v_rel / (invM_a + invM_b)
    float j = -(1.0f + e) * velAlongNormal;
    j /= totalInvMass;

    vec3s impulse = glms_vec3_scale(contact->normal, j);
    if (aBody->invMass > 0.0f && aBody->mass > 0.0f)
    {
        aBody->vel = glms_vec3_add(aBody->vel, glms_vec3_scale(impulse, aBody->invMass));
    }
    if (bBody->invMass > 0.0f && bBody->mass > 0.0f)
    {
        bBody->vel = glms_vec3_sub(bBody->vel, glms_vec3_scale(impulse, bBody->invMass));
    }
}

void Collisions_Dynamic_Hashed(World *world, int idA, Body3Sys *sys, SolXform *xform, SolBody3 *body)
{
    SpatialCell cell = Spatial_Cell_GetNeighbors(xform->pos, sys->dynamic_table.cellSize);

    for (int n = 0; n < 27; n++)
    {
        u32 cellHash = cell.neighborHashes[n];
        u32 entry    = sys->dynamic_table.head[cellHash & (sys->dynamic_table.size - 1)];

        while (entry != SPATIAL_NULL)
        {
            int idB = (int)sys->dynamic_table.value[entry];

            if (idA < idB) // Deduplicate pairs
            {
                SolBody3 *other_body = Sol_Comp_Get(world, idB, SolBody3);

                // Don't collide two immovable bodies
                if ((body->mass > 0.0f || other_body->mass > 0.0f) && Sol_Body3_DoesCollide(body, other_body))
                {
                    SolContact contact;
                    if (shape_pair_test[body->shape][other_body->shape] &&
                        shape_pair_test[body->shape][other_body->shape](world, idA, idB, &contact))
                    {
                        if (sys->contacts.contact_cnt < MAX_CONTACTS - 1)
                        {
                            int idx                        = sys->contacts.contact_cnt++;
                            sys->contacts.contact[idx]     = contact;
                            sys->contacts.contact[idx].id  = idA;
                            sys->contacts.contact[idx].idB = idB;
                        }
                    }
                }
            }
            entry = sys->dynamic_table.next[entry];
        }
    }
}

void Collisions_Dynamic_Tris(World *world, int idA, Body3Sys *sys, SolXform *xform, SolBody3 *body)
{
    SpatialCell cell = Spatial_Cell_GetNeighbors(xform->pos, sys->dynamic_tri_table.cellSize);

    for (int n = 0; n < 27; n++)
    {
        u32 cellHash = cell.neighborHashes[n];
        u32 entry    = sys->dynamic_tri_table.head[cellHash & (sys->dynamic_tri_table.size - 1)];

        while (entry != SPATIAL_NULL)
        {
            u32 val        = sys->dynamic_tri_table.value[entry];
            int modelEntID = GET_TRI_ENTITY(val);

            if (idA != modelEntID && shape_tri_test[body->shape])
            {
                int       triIdx = GET_TRI_INDEX(val);
                SolModel *model  = Sol_Comp_Get(world, modelEntID, SolModel);
                SolXform *xformB = Sol_Comp_Get(world, modelEntID, SolXform);
                if (model)
                {
                    SolTri localTri = loaded_models[model->kind].tris[triIdx];
                    SolTri worldTri = SolTri_GetWorldSpace(&localTri, xformB->rot, xformB->pos, xformB->sca);

                    SolContact contact;
                    if (shape_tri_test[body->shape](world, idA, modelEntID, &worldTri, &contact))
                    {
                        if (sys->contacts.contact_cnt < MAX_CONTACTS - 1)
                        {
                            int idx                        = sys->contacts.contact_cnt++;
                            sys->contacts.contact[idx]     = contact;
                            sys->contacts.contact[idx].id  = idA;
                            sys->contacts.contact[idx].idB = modelEntID;
                        }
                    }
                }
            }
            entry = sys->dynamic_tri_table.next[entry];
        }
    }
}

SubstepData Substep_Get(float speed, float radius, float fdt)
{
    SubstepData substep_data;
    if (radius <= 0.0001f)
    {
        substep_data.sub_dt   = fdt;
        substep_data.substeps = 1;
        return substep_data;
    }

    u8 substeps = (int)ceilf(speed * fdt / radius);
    if (substeps < 1)
        substeps = 1;
    if (substeps > 16)
        substeps = 16;

    substep_data.substeps = substeps;
    substep_data.sub_dt   = fdt / (float)substeps;

    return substep_data;
}

void Body3_Step(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_SolBody3 *set = Sol_Comp_Set(world, SolBody3);
    Body3Sys           *ws  = world->systems[WORLDSYS_BODY3];
    SpatialTable_Clear(&ws->dynamic_table);
    SpatialTable_Clear(&ws->dynamic_tri_table);
    ws->contacts.contact_cnt = 0;

    for (int i = 0; i < set->cnt; i++)
    {
        int       id    = set->dense[i];
        SolBody3 *body3 = &set->data[i];
        SolXform *xform = Sol_Comp_Get(world, id, SolXform);
        switch (body3->shape)
        {
        case SHAPE3_MOD:
            if (Sol_Comp_Has(world, id, SolModel))
            {
                body3->putInTable = true;
                SolModel *model   = Sol_Comp_Get(world, id, SolModel);
                //                Prof_Begin(&body_profiler);
                int                 t;
                const SolModelData *md    = &loaded_models[model->kind];
                const SolTri       *tris  = md->tris;
                int                 count = md->tri_count;
                for (t = 0; t < count; t++)
                {
                    SolTri worldTri = SolTri_GetWorldSpace(&tris[t], xform->rot, xform->pos, xform->sca);
                    Spatial_Hash_Tri(&ws->dynamic_tri_table, &worldTri, MAKE_TRI_VALUE(id, t));
                }
                //              Prof_EndEz(&body_profiler, true);
            }
            break;
            default:
            vec3s min = vecSub(xform->pos, body3->dims);
            vec3s max = vecAdd(xform->pos, body3->dims);
            Spatial_FloorHashInsert(&ws->dynamic_table, min, max, id);
        }
    }

    for (int i = 0; i < set->cnt; i++)
    {
        int       id    = set->dense[i];
        SolBody3 *body3 = &set->data[i];
        if (body3->mass == 0.0f)
            continue;
        SolXform *xform = Sol_Comp_Get(world, id, SolXform);
        if (!xform)
            continue;

        float linearDamping = 0.99f;
        body3->vel.x *= linearDamping;
        body3->vel.z *= linearDamping;

        // Snap small floating values to zero to avoid precision crawl
        if (fabsf(body3->vel.x) < 0.001f)
            body3->vel.x = 0.0f;
        if (fabsf(body3->vel.z) < 0.001f)
            body3->vel.z = 0.0f;

        vec3s accel    = body3->vel.y < TERMINAL_VELOCITY ? GLMS_VEC3_ZERO : body3->gravity;
        accel          = glms_vec3_add(accel, body3->force);
        accel          = glms_vec3_add(accel, body3->impulse);
        body3->impulse = (vec3s){0};
        body3->vel     = glms_vec3_add(body3->vel, glms_vec3_scale(accel, fdt));

        xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body3->vel, fdt));
        Collisions_Dynamic_Hashed(world, id, ws, xform, body3);
        Collisions_Dynamic_Tris(world, id, ws, xform, body3);
        // SubstepData substep = Substep_Get(glms_vec3_norm(body3->vel), fmaxf(body3->dims.x, body3->dims.y) * 0.9f,
        // fdt); for (int s = 0; s < substep.substeps; s++)
        // {
        //     xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body3->vel, substep.sub_dt));
        //     Collisions_Dynamic_Hashed(world, id, ws, xform, body3);
        // }
        if (xform->pos.y < 0)
        {
            xform->pos.y = 0;
            body3->vel.y *= -body3->restitution;
        }
    }

    for (int i = 0; i < ws->contacts.contact_cnt; i++)
    {
        SolContact *contact = &ws->contacts.contact[i];
        int         idA     = contact->id;
        int         idB     = contact->idB;

        SolBody3 *bodyA  = Sol_Comp_Get(world, idA, SolBody3);
        SolXform *xformA = Sol_Comp_Get(world, idA, SolXform);
        SolBody3 *bodyB  = Sol_Comp_Get(world, idB, SolBody3);
        SolXform *xformB = Sol_Comp_Get(world, idB, SolXform);

        if (bodyA && xformA)
        {
            Resolve_Dynamic_Pair(bodyA, xformA, bodyB, xformB, contact);
        }
    }
}

void Body3_Init(World *world)
{
    Body3Sys *ws                   = malloc(sizeof(Body3Sys));
    world->systems[WORLDSYS_BODY3] = ws;
    SpatialTable_Init(&ws->dynamic_table, SPATIAL_SIZE, SPATIAL_CAP, SPATIAL_CELL_SIZE);
    SpatialTable_Init(&ws->dynamic_tri_table, SPATIAL_TRI_SIZE, SPATIAL_TRI_CAP, SPATIAL_TRI_CELL_SIZE);
}

void Body3_Deinit(World *world)
{
    Body3Sys *ws = world->systems[WORLDSYS_BODY3];
    if (ws)
    {
        SpatialTable_Free(&ws->dynamic_table);
        SpatialTable_Free(&ws->dynamic_tri_table);
        free(ws);
        world->systems[WORLDSYS_BODY3] = NULL;
    }
}

bool Sol_Body3_DoesCollide(SolBody3 *body, SolBody3 *other_body)
{
    return (body->group << 16) & (other_body->group);
}

vec3s Sol_Body3_GetGround(World *world, int id)
{
    return GLMS_VEC3_ZERO;
}

int Sol_Body3_Raycast(World *world, SolRay ray, SolRayResult *result, int max)
{
    return 0;
}
