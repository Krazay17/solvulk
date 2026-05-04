#include "sol_core.h"

#define MAX_PITCH (GLM_PI_2 - 0.01f)
float lookSens = 0.001f;

static const PlayerActionStates action_binds[SOL_KEY_COUNT] = {
    [SOL_KEY_0] = ACTION_ABILITY0, [SOL_KEY_1] = ACTION_ABILITY1, [SOL_KEY_2] = ACTION_ABILITY2,
    [SOL_KEY_3] = ACTION_ABILITY3, [SOL_KEY_4] = ACTION_ABILITY4, [SOL_KEY_5] = ACTION_ABILITY5,
    [SOL_KEY_6] = ACTION_ABILITY6, [SOL_KEY_7] = ACTION_ABILITY7, [SOL_KEY_8] = ACTION_ABILITY8,
    [SOL_KEY_9] = ACTION_ABILITY9, [SOL_KEY_W] = ACTION_FWD,      [SOL_KEY_A] = ACTION_LEFT,
    [SOL_KEY_S] = ACTION_BWD,      [SOL_KEY_D] = ACTION_RIGHT,    [SOL_KEY_F] = 0,
    [SOL_KEY_SPACE] = ACTION_JUMP, [SOL_KEY_ESCAPE] = 0,          [SOL_KEY_SHIFT] = ACTION_DASH,
};

typedef struct CompController
{
    vec3s              lookdir, wishdir, aimdir, aimpos, aimHitPos;
    vec2s              wishdir2;
    PlayerActionStates actionState;
    float              yaw, pitch;
    float              zoom;
    u32                aimHitEnt;
    ControllerKind     kind;
} CompController;

static void  LocalTick(World *world, int id, double dt, double time);
static void  AiTick(World *world, double dt, double time);
static vec3s GetWishDir3(uint32_t action, vec3s lookdir, vec3s updir);
static vec2s GetWishDir2(uint32_t action);

void Sol_Controller_Add(World *world, int id, ControllerDesc desc)
{
    world->masks[id] |= HAS_CONTROLLER;

    if (desc.kind == CONTROLLER_LOCAL)
        world->playerID = id;
}

void Sol_Controller_Tick(World *world, double dt, double time)
{
    int required = HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        if (world->controllers[id].kind == CONTROLLER_LOCAL)
            LocalTick(world, id, dt, time);
        AiTick(world, dt, time);
    }
}

static void LocalTick(World *world, int id, double dt, double time)
{
    SolMouse        mouse      = Sol_Input_GetMouse();
    CompController *controller = &world->controllers[id];
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

    vec3s lookdir = glms_vec3_normalize(Sol_Vec3_FromYawPitch(controller->yaw, controller->pitch));
    vec3s wishdir = GetWishDir3(controller->actionState, lookdir, WORLD_UP);

    controller->lookdir  = lookdir;
    controller->wishdir  = wishdir;
    controller->wishdir2 = GetWishDir2(controller->actionState);

    if (Sol_Input_KeyDown(SOL_KEY_F))
    {
        vec3s pos = glms_vec3_add(*Sol_Xform_GetPos(world, id), glms_vec3_scale(lookdir, (float)dt * 60.0f));
        Sol_Xform_SetPos(world, id, pos);
        Sol_Physx_SetVel(world, id, (vec3s){0, 0, 0});
    }
}

static void AiTick(World *world, double dt, double time)
{
    int required = HAS_CONTROLLER_AI;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompController *controller = &world->controllers[id];
        vec3s           myPos      = *Sol_Xform_GetPos(world, id);
        vec3s           playerPos  = *Sol_Xform_GetPos(world, world->playerID);
        vec3s           lookdir    = glms_vec3_normalize(glms_vec3_sub(playerPos, myPos));
        float           yaw        = atan2f(lookdir.x, lookdir.z);
        float           pitch      = asinf(lookdir.y);

        controller->lookdir = lookdir;
        controller->wishdir = lookdir;
        controller->yaw     = yaw;
        controller->pitch   = pitch;

        Sol_Xform_SetYaw(world, id, yaw);
    }
}

static vec3s GetWishDir3(uint32_t action, vec3s lookdir, vec3s updir)
{
    vec3s wishdir  = {0, 0, 0};
    vec3s flatdir  = lookdir;
    flatdir.y      = 0;
    vec3s rightdir = glms_vec3_normalize(glms_vec3_cross(flatdir, updir));
    if (action & ACTION_FWD)
        wishdir = glms_vec3_add(wishdir, flatdir);

    if (action & ACTION_BWD)
        wishdir = glms_vec3_sub(wishdir, flatdir);

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