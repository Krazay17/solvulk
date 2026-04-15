#include <cglm/struct.h>
#include <omp.h>

#include "sol_core.h"

void Sol_System_Step_Physx_3d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY3 | HAS_XFORM;
    WorldSpatial *ws = &world->worldSpatial;

    int count = world->activeCount;
    int i;
#pragma omp parallel for if (count > 1000)
    for (i = 0; i < count; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        vec3s accel = SOL_PHYS_GRAV;
        accel = glms_vec3_add(accel, body->force);
        accel = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};

        body->vel = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));
        xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, fdt));
    }

    // 2. Rebuild dynamic spatial after integration
    Sol_System_Spatial_Step(world, dt, 0);

    // 3. Collision resolution
    // Temp buffer to track which triangles this entity already checked
    static u32 checkedTris[4096];
    static u32 checkedCount = 0;

    int j;
#pragma omp parallel for if (count > 1000)
    for (j = 0; j < world->activeCount; j++)
    {
        int id = world->activeEntities[j];
        if ((world->masks[id] & required) != required)
            continue;

        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];
        if (body->invMass <= 0.0f)
            continue;

        vec3s pos = xform->pos;
        int ix = (int)floorf(pos.x / SPATIAL_CELL_SIZE);
        int iy = (int)floorf(pos.y / SPATIAL_CELL_SIZE);
        int iz = (int)floorf(pos.z / SPATIAL_CELL_SIZE);

        // Reset dedup for this entity
        checkedCount = 0;
        body->grounded = 0;

        for (int ox = -1; ox <= 1; ox++)
            for (int oy = -1; oy <= 1; oy++)
                for (int oz = -1; oz <= 1; oz++)
                {
                    u32 hash = HashCoords(ix + ox, iy + oy, iz + oz);

                    // A. Static triangles
                    u32 entry = ws->staticWorld.head[hash];
                    while (entry != SPATIAL_NULL)
                    {
                        u32 triIdx = ws->staticWorld.value[entry];

                        // Dedup: skip if we already checked this triangle
                        bool skip = false;
                        for (u32 c = 0; c < checkedCount; c++)
                        {
                            if (checkedTris[c] == triIdx)
                            {
                                skip = true;
                                break;
                            }
                        }

                        if (!skip && checkedCount < 4096)
                        {
                            checkedTris[checkedCount++] = triIdx;
                            CollisionTri *tri = &ws->tris[triIdx];
                            SolCollision collision = ResolveSphereTriangle(body, &xform->pos, tri);
                            if (collision.didCollide && collision.normal.y > 0.5f)
                                body->grounded = 1;
                        }

                        entry = ws->staticWorld.next[entry];
                    }

                    // B. Dynamic entities
                    u32 dynEntry = ws->dynamicUnits.head[hash];
                    while (dynEntry != SPATIAL_NULL)
                    {
                        u32 otherID = ws->dynamicUnits.value[dynEntry];
                        if (id < (int)otherID)
                            ResolveCollision(body, xform, &world->bodies[otherID], &world->xforms[otherID]);

                        dynEntry = ws->dynamicUnits.next[dynEntry];
                    }
                }
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
