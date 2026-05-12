/*
 * File: s_controller.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Local Tick reads inputs and sets action states.
 * Remote Tick sets action states directly.
 * Ai Tick sets action states directly.
 */

#include "sol_core.h"

typedef struct CompController
{
    vec3s          lookdir, wishdir, aimdir, aimpos, aimHitPos;
    vec2s          wishdir2;
    SolActions     actionState;
    float          yaw, pitch;
    float          zoom;
    u32            aimHitEnt;
    ControllerKind kind;
} CompController;

static const SolActions action_binds[SOL_KEY_COUNT] = {
    [SOL_KEY_0] = ACTION_ABILITY0, [SOL_KEY_1] = ACTION_ABILITY1, [SOL_KEY_2] = ACTION_ABILITY2,
    [SOL_KEY_3] = ACTION_ABILITY3, [SOL_KEY_4] = ACTION_ABILITY4, [SOL_KEY_5] = ACTION_ABILITY5,
    [SOL_KEY_6] = ACTION_ABILITY6, [SOL_KEY_7] = ACTION_ABILITY7, [SOL_KEY_8] = ACTION_ABILITY8,
    [SOL_KEY_9] = ACTION_ABILITY9, [SOL_KEY_W] = ACTION_FWD,      [SOL_KEY_A] = ACTION_LEFT,
    [SOL_KEY_S] = ACTION_BWD,      [SOL_KEY_D] = ACTION_RIGHT,    [SOL_KEY_F] = 0,
    [SOL_KEY_SPACE] = ACTION_JUMP, [SOL_KEY_ESCAPE] = 0,          [SOL_KEY_SHIFT] = ACTION_DASH,
};

static void  LocalTick(World *world, int id, double dt, double time);
static void  AiTick(World *world, int id, double dt, double time);
static vec3s GetWishDir3(uint32_t action, vec3s lookdir, vec3s updir);
static vec2s GetWishDir2(uint32_t action);

void Sol_Controller_Init(World *world)
{
    world->tickSystems[world->tickCount++] = Sol_Controller_Tick;

    world->controllers = calloc(MAX_ENTS, sizeof(CompController));
}

void Sol_Controller_Add(World *world, int id, ControllerDesc desc)
{
    world->masks[id] |= HAS_CONTROLLER;
    CompController *controller = &world->controllers[id];
    controller->kind           = desc.kind;
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
        else if (world->controllers[id].kind == CONTROLLER_AI)
            AiTick(world, id, dt, time);
    }
}

static void LocalTick(World *world, int id, double dt, double time)
{
    CompController *controller = &world->controllers[id];
    SolLook        *look       = Sol_Input_GetLook();

    for (int i = 0; i < SOL_KEY_COUNT; i++)
    {
        if (Sol_Input_KeyDown(i))
            controller->actionState |= action_binds[i];
        else
            controller->actionState &= ~action_binds[i];
    }

    vec3s head = world->xforms[id].drawPos;
    head.y += world->bodies[id].dims.y * 0.4f;
    Sol_Cam_Arm_Update(world, head, dt);

    controller->yaw      = look->yaw;
    controller->pitch    = look->pitch;
    controller->lookdir  = look->lookdir;
    controller->wishdir  = GetWishDir3(controller->actionState, look->lookdir, WORLD_UP);
    controller->wishdir2 = GetWishDir2(controller->actionState);

    controller->aimHitEnt = -1;
    SolRayResult aimTrace = Sol_Raycast(world, (SolRay){
                                                   .pos       = Sol_Cam_GetArm()->arm,
                                                   .ignoreEnt = id,
                                                   .mask      = 0b01,
                                                   .dir       = look->lookdir,
                                                   .dist      = 60.f,
                                               });
    vec3s        dir      = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, head));
    controller->aimdir    = vecDot(dir, look->lookdir) > 0.6f ? dir : look->lookdir;
    controller->aimHitEnt = aimTrace.entId;
    controller->aimpos    = head;

    // Debug Teleport
    if (Sol_Input_KeyDown(SOL_KEY_F))
    {
        vec3s pos = glms_vec3_add(Sol_Xform_GetPos(world, id),
                                  glms_vec3_scale(Sol_Input_GetLook()->lookdir, (float)dt * 60.0f));
        Sol_Xform_Teleport(world, id, pos);
        Sol_Physx_SetVel(world, id, (vec3s){0, 0, 0});
    }
}

static void AiTick(World *world, int id, double dt, double time)
{
    CompController *controller = &world->controllers[id];
    vec3s           myPos      = Sol_Xform_GetPos(world, id);
    vec3s           playerPos  = Sol_Xform_GetPos(world, world->playerID);
    vec3s           lookdir    = glms_vec3_normalize(glms_vec3_sub(playerPos, myPos));
    float           yaw        = atan2f(lookdir.x, lookdir.z);
    float           pitch      = asinf(lookdir.y);

    Sol_Xform_SetYaw(world, id, yaw);
    controller->lookdir = lookdir;
    controller->wishdir = lookdir;
    controller->yaw     = yaw;
    controller->pitch   = pitch;

    if (((int)time % 6) == 0)
    {
        controller->actionState |= ACTION_JUMP;
    }
    else
        controller->actionState &= ~ACTION_JUMP;
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

vec3s Sol_Controller_GetAimPos(World *world, int id)
{
    CompController *controller = &world->controllers[id];
    return vecAdd(controller->aimpos, vecSca(controller->aimdir, 0.5f));
}

SolActions Sol_GetActions(World *world, int id)
{
    return world->controllers[id].actionState;
}

vec3s Sol_GetWishdir(World *world, int id)
{
    return world->controllers[id].wishdir;
}

vec2s Sol_GetWishdir2(World *world, int id)
{
    return world->controllers[id].wishdir2;
}

vec3s Sol_GetLookdir(World *world, int id)
{
    return world->controllers[id].lookdir;
}

vec3s Sol_GetAimpos(World *world, int id)
{
    return world->controllers[id].aimpos;
}

vec3s Sol_GetAimdir(World *world, int id)
{
    return world->controllers[id].aimdir;
}

float Sol_GetYaw(World *world, int id)
{
    return world->controllers[id].yaw;
}
float Sol_GetPitch(World *world, int id)
{
    return world->controllers[id].pitch;
}

StrafeDir Sol_GetStrafedir(World *world, int id, float xIn, float zIn)
{
    float x = xIn ? xIn : world->controllers[id].wishdir.x;
    float z = zIn ? zIn : world->controllers[id].wishdir.z;

    float xB = world->controllers[id].lookdir.x;
    float zB = world->controllers[id].lookdir.z;

    float angle = atan2f(x, z) - atan2f(xB, zB);
    if (angle < 0)
        angle += 2.0f * M_PI;

    return (int)((angle / (M_PI / 2.0f)) + 0.5f) % 4;
}