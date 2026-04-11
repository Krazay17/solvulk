#include <cglm/struct.h>
#include <immintrin.h>

#include "sol_core.h"

#define SOL_GRAVITY 9.81f

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
                                   : body->vel.y + SOL_GRAVITY * fdt;
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
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompBody *body = &world->bodies[id];
            CompXform *xform = &world->xforms[id];
            CompModel *model = &world->models[id];

            body->vel.y -= SOL_GRAVITY * fdt;
            vec3s finalVel = glms_vec3_scale(body->vel, fdt);
            xform->pos = glms_vec3_add(xform->pos, finalVel);
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
    }
}

typedef struct
{
    float min[4];
    float max[4];
} AABB3D;

bool Collision3d(AABB3D *a)
{
    //__m128
    return false;
}