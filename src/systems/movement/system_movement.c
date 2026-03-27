#include "sol_core.h"

static vec3s GetWishDir(uint32_t action, vec3s lookdir, vec3s updir)
{
    vec3s wishdir = {0,0,0};
    vec3s rightdir = glms_vec3_normalize(glms_vec3_cross(lookdir, updir));
    if (action & ACTION_FWD)
        glms_vec3_add(wishdir, lookdir);

    if (action & ACTION_BWD)
        glms_vec3_sub(wishdir, lookdir);

    if (action & ACTION_RIGHT)
        glms_vec3_add(wishdir, rightdir);

    if (action & ACTION_LEFT)
        glms_vec3_sub(wishdir, rightdir);
    return wishdir;
}

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
            vec3s lookdir = glms_vec3_normalize(Sol_Vec3_FromYawPitch(controller->yaw, controller->pitch));
            vec3s updir = glms_vec3_normalize(glms_vec3_norm2(movement->updir) > 0 ? movement->updir : (vec3s){0, 1, 0});
            vec3s wishdir = GetWishDir(controller->actionState, lookdir, updir);

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
            vec3s wishdir = {0, 0, 0};
            wishdir.x += controller->actionState & ACTION_RIGHT  ? 1.0f
                         : controller->actionState & ACTION_LEFT ? -1.0f
                                                                 : 0;

            body->vel = glms_vec3_add(body->vel, glms_vec3_scale(wishdir, movement->gAccell * fdt));
        }
    }
}
