#include "controller.h"
#include "sol_core.h"

#define MAX_PITCH (GLM_PI_2 - 0.01f)

float lookSens = 0.001f;

static vec3s GetWishDir3(uint32_t action, vec3s lookdir, vec3s updir);
static vec2s GetWishDir2(uint32_t action);

CompController *Sol_ControllerLocal_Add(World *world, int id)
{
    world->playerID = id;

    world->masks[id] |= HAS_CONTROLLER;
    return &world->controllers[id];
}

void Sol_System_Controller_Local_Tick(World *world, double dt, double time)
{
    int id = world->playerID;
    if (id < 0)
        return;
    SolMouse        mouse      = Sol_Input_GetMouse();
    CompXform      *xform      = &world->xforms[id];
    CompController *controller = &world->controllers[id];
    CompMovement   *movement   = &world->movements[id];
    CompBody       *body       = &world->bodies[id];
    CompCombat     *combat     = &world->combats[id];
    float           yaw        = controller->yaw;
    float           pitch      = controller->pitch;

    if (mouse.locked)
    {
        yaw -= mouse.dx * lookSens;
        pitch -= mouse.dy * lookSens;

        yaw = fmodf(yaw, 2.0f * GLM_PI);
        if (yaw > GLM_PI)
            yaw -= 2.0f * GLM_PI;
        else if (yaw < -GLM_PI)
            yaw += 2.0f * GLM_PI;

        if (pitch > MAX_PITCH)
            pitch = MAX_PITCH;
        if (pitch < -MAX_PITCH)
            pitch = -MAX_PITCH;
    }

    controller->yaw   = yaw;
    controller->pitch = pitch;

    for (int i = 0; i < SOL_KEY_COUNT; i++)
    {
        if (Sol_Input_KeyDown(i))
            controller->actionState |= action_binds[i];
        else
            controller->actionState &= ~action_binds[i];
    }

    if (Sol_Input_GetMouse().buttons[SOL_MOUSE_LEFT])
        controller->actionState |= ACTION_ATTACK;
    else
        controller->actionState &= ~ACTION_ATTACK;

    vec3s lookdir = glms_vec3_normalize(Sol_Vec3_FromYawPitch(controller->yaw, controller->pitch));
    vec3s updir   = glms_vec3_normalize(glms_vec3_norm2(movement->updir) > 0 ? movement->updir : (vec3s){0, 1, 0});
    vec3s wishdir = GetWishDir3(controller->actionState, lookdir, updir);

    if (body && combat)
    {
        combat->attackPos = xform->pos;
        combat->attackPos.y += body->height * 0.5f;
    }

    controller->lookdir  = lookdir;
    controller->wishdir  = wishdir;
    controller->wishdir2 = GetWishDir2(controller->actionState);

    if (Sol_Input_KeyDown(SOL_KEY_F))
    {
        world->xforms[id].pos = glms_vec3_add(world->xforms[id].pos, glms_vec3_scale(lookdir, (float)dt * 60.0f));
        world->bodies[id].vel = (vec3s){0, 0, 0};
    }
}

static vec3s GetWishDir3(uint32_t action, vec3s lookdir, vec3s updir)
{
    vec3s wishdir  = {0, 0, 0};
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