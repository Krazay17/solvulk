#include <cglm/struct.h>

#include "sol_core.h"

static const MoveConfig MOVE_CONFIGS[MOVE_CONFIG_COUNT] = {
    [MOVE_CONFIG_PLAYER] = {.speed = 5.0f, .accell = 10.0f, .fritction = 5.0f},
    [MOVE_CONFIG_WIZARD] = {.speed = 3.5f, .accell =  6.0f, .fritction = 5.0f},
}

void Sol_System_Movement_3d_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;

    int required = HAS_MOVEMENT | HAS_XFORM | HAS_BODY3 | HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompMovement *movement = &world->movements[id];
            CompBody *body = &world->bodies[id];
            CompController *controller = &world->controllers[id];
            MoveConfig *cfg = &MOVE_CONFIGS[movement.configId];

            vec3s wishdir = controller->wishdir;
            vec3s vel = body->vel;
            
            switch(movement.moveState)
            {
                default:
                    vel = glms_vec3_add(vel, glms_vec3_scale(wishdir, cfg->accell * fdt));
            }
    
            body->vel = vel;
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
