#include <cglm/struct.h>

#include "sol_core.h"

float lookSens = 0.001f;
static bool mouseLocked = false;

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

    if (action & ACTION_JUMP)
        wishdir = glms_vec3_add(wishdir, updir);

    return glms_vec3_normalize(wishdir);
}

static vec2s GetWishDir2(uint32_t action)
{
    vec2s wishdir = {0, 0};
    if (action & ACTION_RIGHT)
        wishdir.x += 1;
    if (action & ACTION_LEFT)
        wishdir.x -= 1;
        
    return glms_vec2_normalize(wishdir);
}

void Sol_System_Controller_Local_Tick(World *world, double dt, double time)
{
    SolMouse mouse = SolInput_GetMouse();
    int required = HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompController *controller = &world->controllers[id];
            CompMovement *movement = &world->movements[id];

            if (mouse.locked)
            {
                // 1. Apply movement
                controller->yaw -= mouse.dx * lookSens;

                // 2. Wrap to [0, 2pi]
                controller->yaw = fmodf(controller->yaw, 2.0f * GLM_PI);

                // 3. Offset to [-pi, pi]
                if (controller->yaw > GLM_PI)
                    controller->yaw -= 2.0f * GLM_PI;
                else if (controller->yaw < -GLM_PI)
                    controller->yaw += 2.0f * GLM_PI;
                    
                controller->pitch -= mouse.dy * lookSens;
                float maxPitch = GLM_PI_2 - 0.1f;
                if (controller->pitch > maxPitch)
                    controller->pitch = maxPitch;
                if (controller->pitch < -maxPitch)
                    controller->pitch = -maxPitch;
            }
            Sol_Debug_Add("Yaw", controller->yaw);
            Sol_Debug_Add("Pitch", controller->pitch);

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

            if (SolInput_KeyDown(SOL_KEY_SPACE))
                controller->actionState |= ACTION_JUMP;
            else
                controller->actionState &= ~ACTION_JUMP;

            vec3s lookdir = glms_vec3_normalize(Sol_Vec3_FromYawPitch(controller->yaw, controller->pitch));
            vec3s updir = glms_vec3_normalize(glms_vec3_norm2(movement->updir) > 0 ? movement->updir : (vec3s){0, 1, 0});
            vec3s wishdir = GetWishDir3(controller->actionState, lookdir, updir);

            controller->lookdir = lookdir;
            controller->wishdir = wishdir;
            controller->wishdir2 = GetWishDir2(controller->actionState);

            Sol_Debug_Add("LookX", controller->lookdir.x);
            Sol_Debug_Add("LookY", controller->lookdir.y);
            Sol_Debug_Add("LookZ", controller->lookdir.z);
        }
    }
}

void Sol_System_Controller_Ai_Tick(World *world, double dt, double time)
{
}