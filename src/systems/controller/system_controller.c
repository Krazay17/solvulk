#include <cglm/struct.h>

#include "sol_core.h"

float lookSens = 0.001f;

static vec3s GetWishDir3(uint32_t action, vec3s lookdir, vec3s updir)
{
    vec3s wishdir = {0, 0, 0};
    vec3s rightdir = glms_vec3_normalize(glms_vec3_cross(lookdir, updir));
    if (action & ACTION_FWD)
        wishdir = glms_vec3_add(wishdir, lookdir);

    if (action & ACTION_BWD)
        wishdir = glms_vec3_sub(wishdir, lookdir);

    if (action & ACTION_RIGHT)
        wishdir = glms_vec3_add(wishdir, rightdir);

    if (action & ACTION_LEFT)
        wishdir = glms_vec3_sub(wishdir, rightdir);
    return wishdir;
}

static vec2s GetWishDir2(uint32_t action)
{
    vec2s wishdir = {0, 0};
    if (action & ACTION_RIGHT)
        wishdir.x += 1;
    if (action & ACTION_LEFT)
        wishdir.x -= 1;
    return wishdir;
}

void Sol_System_Controller_Local_Tick(World *world, double dt, double time)
{
    SolMouse mouse = SolInput_GetMouse();
    // static int rawMouseX;
    // rawMouseX += mouse.dx;
    // Sol_Debug_Add("Mouse Delta", rawMouseX);
    int required = HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompController *controller = &world->controllers[id];
            CompMovement *movement = &world->movements[id];
            controller->yaw += mouse.dx * lookSens;
            controller->pitch += mouse.dy * lookSens;

            if (SolInput_KeyDown(SOL_KEY_W))
                controller->actionState |= ACTION_FWD;
            else
                controller->actionState &= ~ACTION_FWD;

            if (SolInput_KeyDown(SOL_KEY_S))
                controller->actionState |= ACTION_BWD;
            else
                controller->actionState &= ~ACTION_BWD;

            if (SolInput_KeyDown(SOL_KEY_A))
                controller->actionState |= ACTION_LEFT;
            else
                controller->actionState &= ~ACTION_LEFT;

            if (SolInput_KeyDown(SOL_KEY_D))
                controller->actionState |= ACTION_RIGHT;
            else
                controller->actionState &= ~ACTION_RIGHT;

            vec3s lookdir = glms_vec3_normalize(Sol_Vec3_FromYawPitch(controller->yaw, controller->pitch));
            vec3s updir = glms_vec3_normalize(glms_vec3_norm2(movement->updir) > 0 ? movement->updir : (vec3s){0, 1, 0});
            vec3s wishdir = GetWishDir3(controller->actionState, lookdir, updir);

            controller->lookdir = lookdir;
            controller->wishdir = wishdir;
            controller->wishdir2 = GetWishDir2(controller->actionState);

            Sol_Debug_Add("Look", controller->lookdir.x);
            Sol_Debug_Add("Move", controller->wishdir.x);
                }
    }
}

void Sol_System_Controller_Ai_Tick(World *world, double dt, double time)
{
}