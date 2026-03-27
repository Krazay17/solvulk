#include "sol.h"
#include <stdlib.h>
#include <cglm/cglm.h>

#define SOL_GRAVITY 9.81f

void Sol_System_Step_Physx_2d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY | HAS_XFORM;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompBody *body = &world->bodies[id];
            CompXform *xform = &world->xforms[id];
            
            body->vel.y += SOL_GRAVITY * fdt;
            vec3s finalVel = glms_vec3_scale(body->vel, fdt * 20.0f);
            xform->pos = glms_vec3_add(xform->pos, finalVel);

            if(xform->pos.y + body->height >= solState.windowHeight)
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
    int required = HAS_BODY | HAS_XFORM | HAS_MODEL;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompBody *body = &world->bodies[id];
            CompXform *xform = &world->xforms[id];
            CompModel *model = &world->models[id];
            
            body->vel.y += SOL_GRAVITY * fdt;
            vec3s finalVel = glms_vec3_scale(body->vel, fdt);
            xform->pos = glms_vec3_add(xform->pos, finalVel);
        }
    }
}