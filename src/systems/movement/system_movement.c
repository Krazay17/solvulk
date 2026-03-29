#include <cglm/struct.h>

#include "sol_core.h"

void Sol_System_Movement_3d_Step(World *world, double dt, double time)
{
    int required = HAS_MOVEMENT | HAS_XFORM | HAS_BODY3 | HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompMovement *movement = &world->movements[id];
            CompBody *body = &world->bodies[id];
            CompController *controller = &world->controllers[id];

            vec3s wishdir = controller->wishdir;

            body->vel = glms_vec3_add(body->vel, glms_vec3_scale(wishdir, movement->gAccell));
        }
    }
}

void Sol_System_Movement_2d_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_MOVEMENT | HAS_XFORM | HAS_BODY2 | HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompMovement *movement = &world->movements[id];
            CompBody *body = &world->bodies[id];
            CompController *controller = &world->controllers[id];
            
            vec2s wishdir = controller->wishdir2;

            body->vel = glms_vec3_add(body->vel, glms_vec3_scale((vec3s){wishdir.x, wishdir.y, 0}, movement->gAccell * fdt));
        }
    }
}
