#include <cglm/struct.h>
#include <omp.h>

#include "sol_core.h"

static int         ents[MAX_ENTS];
static SolProfiler prof_static  = {.name = "Static"};
static SolProfiler prof_dynamic = {.name = "Dynamic"};
static SolProfiler prof_inner   = {.name = "Inner"};
static int         prof_frame   = 0;

void Physx_Init(World *world)
{
    world->spatial = calloc(1, sizeof(WorldSpatial));
    SpatialTable_Init(&world->spatial->staticGroup.table, SPATIAL_STATIC_SIZE, SPATIAL_STATIC_ENTRIES,
                      SPATIAL_STATIC_CELL_SIZE);
    SpatialTable_Init(&world->spatial->dynamicGroup.table, SPATIAL_DYNAMIC_SIZE, SPATIAL_DYNAMIC_ENTRIES,
                      SPATIAL_DYNAMIC_CELL_SIZE);
}

CompBody *Entity_Add_Body2(World *world, int id)
{
    world->masks[id] |= HAS_BODY2;
    return &world->bodies[id];
}

CompBody *Sol_Physx_Add(World *world, int id, CompBody init_body)
{
    CompBody body = init_body;
    body.height   = body.height ? body.height : 0.5f;
    body.radius   = body.radius ? body.radius : 0.5f;
    body.length   = body.length ? body.length : 0.5f;
    body.invMass  = body.mass > 0 ? 1.0f / body.mass : 0;

    if (body.shape == SHAPE3_MOD)
    {
        if (body.mass == 0)
        {
            Spatial_Tris_Add_Model(&world->spatial->staticGroup, id, world->models[id].model, &world->xforms[id]);
        }
    }

    world->bodies[id] = body;
    world->masks[id] |= HAS_BODY3;
    return &world->bodies[id];
}

void Physx_Step(World *world, double dt, double time)
{
    if (Sol_GetState()->fps < 30)
        return;
    float          fdt      = (float)dt;
    int            required = HAS_BODY3 | HAS_XFORM;
    int            i, j, k, l;
    int            count        = 0;
    int            activeCount  = world->activeCount;
    WorldSpatial  *ws           = world->spatial;
    WorldTriGroup *staticGroup  = &ws->staticGroup;
    WorldTriGroup *dynamicGroup = &ws->dynamicGroup;

    // Filter
    for (i = 0; i < activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        ents[count++] = id;
    }
    Prof_Begin(&prof_static);
    // velocity and static collisions
#pragma omp parallel for if (count > 500) schedule(dynamic, 16)
    for (j = 0; j < count; j++)
    {
        int       id   = ents[j];
        CompBody *body = &world->bodies[id];
        if (body->mass == 0)
            continue;
        CompXform *xform = &world->xforms[id];
        body->grounded   = 0;

        vec3s accel   = SOL_PHYS_GRAV;
        accel         = glms_vec3_add(accel, body->force);
        accel         = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel     = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));

        ResolveShapeTri resolver = shape_tri_resolvers[body->shape];

        SubstepData substep = Substep_Get(body, fdt);
        for (int s = 0; s < substep.substeps; s++)
        {
            xform->pos       = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, substep.sub_dt));
            SolCollision col = Collide_Static_Hashed(staticGroup, body, xform, resolver);
        }
    }

    Prof_End(&prof_static);
    if (prof_frame++ % 200 == 0)
    {
        Prof_Print(&prof_static);
    }
}

SolCollision Collide_Static_Hashed(WorldTriGroup *group, CompBody *body, CompXform *xform, ResolveShapeTri resolver)
{
    int          checks = 0;
    SpatialCell  cell   = Spatial_Cell_Get(xform->pos, group->table.cellSize);
    SolCollision col    = {0};
    for (int c = 0; c < 27; c++)
    {
        u32 entry = group->table.head[cell.neighborHashes[c] & (group->table.size - 1)];
        while (entry != SPATIAL_NULL)
        {
            if (checks > 0xff)
                break;
            SolTri *tri = &group->tris[group->table.value[entry]];
            col         = resolver(body, xform, tri);
            if (col.didCollide && col.normal.y > 0.5f)
                body->grounded = 1;

            entry = group->table.next[entry];
        }
    }
    return col;
}

// void Physx_Step(World *world, double dt, double time) {
//   if (Sol_GetState()->fps < 30)
//     return;
//   float         fdt      = (float)dt;
//   int           required = HAS_BODY3 | HAS_XFORM;
//   int           i, j, k, l;
//   int           count   = 0;
//   int           actives = world->activeCount;
//   WorldSpatial *ws      = world->spatial;

//   // Filter entities
//   for (k = 0; k < actives; k++) {
//     int id = world->activeEntities[k];
//     if ((world->masks[id] & required) != required)
//       continue;
//     if (world->bodies[id].mass == 0)
//       continue;
//     ents[count++] = id;
//   }

//   rebuild_grid_static(ws);

//   // Static resolution
//   Prof_Begin(&prof_static);
// #pragma omp parallel for if (count > 500) schedule(dynamic, 16)
//   for (i = 0; i < count; i++) {
//     int        id    = ents[i];
//     CompXform *xform = &world->xforms[id];
//     CompBody  *body  = &world->bodies[id];

//     if (xform->pos.y < -15.0f) {
//       xform->pos = (vec3s){0, 100, 0};
//       continue;
//     }

//     body->grounded = 0;

//     vec3s accel   = SOL_PHYS_GRAV;
//     accel         = glms_vec3_add(accel, body->force);
//     accel         = glms_vec3_add(accel, body->impulse);
//     body->impulse = (vec3s){0};
//     body->vel     = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));

//     collisions_grid_static(body, xform, ws, fdt);
//   }

//   Prof_End(&prof_static);

//   // ReBuild dynamic spatial table
//   SpatialTable_Clear(&world->spatial->table_dynamic);
//   for (l = 0; l < count; l++) {
//     int   id   = ents[l];
//     vec3s pos  = world->xforms[id].pos;
//     int   ix   = (int)floorf(pos.x / SPATIAL_DYNAMIC_CELL_SIZE);
//     int   iy   = (int)floorf(pos.y / SPATIAL_DYNAMIC_CELL_SIZE);
//     int   iz   = (int)floorf(pos.z / SPATIAL_DYNAMIC_CELL_SIZE);
//     u32   hash = hash_coords(ix, iy, iz) & SPATIAL_DYNAMIC_MASK;
//     SpatialTable_Insert(&world->spatial->table_dynamic, hash, (u32)id);
//   }

//   // Dynamic resolution
//   Prof_Begin(&prof_dynamic);
// #pragma omp parallel for if (count > 500) schedule(dynamic, 16)
//   for (j = 0; j < count; j++) {
//     int        id    = ents[j];
//     CompBody  *body  = &world->bodies[id];
//     CompXform *xform = &world->xforms[id];

//     collisions_hash_dynamic(world, id, body, xform);
//   }
//   Prof_End(&prof_dynamic);

//   prof_frame++;
//   if (prof_frame % 200 == 0) {
//     Prof_Print(&prof_static);
//     Prof_Print(&prof_dynamic);
//   }
// }

void Sol_System_Step_Physx_2d(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_BODY2 | HAS_XFORM;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompBody     *body     = &world->bodies[id];
            CompMovement *movement = &world->movements[id];
            CompXform    *xform    = &world->xforms[id];

            float moveGrav = MOVE_STATE_FORCES[movement->configId][movement->moveState].gravity;
            body->vel.y    = moveGrav ? body->vel.y + moveGrav * fdt : body->vel.y + SOL_PHYS_GRAV.y * fdt;
            vec3s finalVel = glms_vec3_scale(body->vel, fdt * 20.0f);
            xform->pos     = glms_vec3_add(xform->pos, finalVel);

            if (xform->pos.y + body->height >= Sol_GetState()->windowHeight)
            {
                xform->pos.y = Sol_GetState()->windowHeight - body->height;
                body->vel.y  = 0;
            }
        }
    }
}
