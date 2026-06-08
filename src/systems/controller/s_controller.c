/*
 * File: s_controller.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Local Tick reads inputs and sets action states.
 * Remote Tick sets action states directly.
 */
#include "sol_core.h"

static const SolActions action_binds[SOL_KEY_COUNT] = {
    [SOL_KEY_Q] = ACTION_ABILITY1,  [SOL_KEY_E] = ACTION_ABILITY2,

    [SOL_KEY_1] = ACTION_ABILITY3,  [SOL_KEY_2] = ACTION_ABILITY4, [SOL_KEY_3] = ACTION_ABILITY5,
    [SOL_KEY_4] = ACTION_ABILITY6,  [SOL_KEY_5] = ACTION_ABILITY7, [SOL_KEY_6] = ACTION_ABILITY8,
    [SOL_KEY_7] = ACTION_ABILITY9,  [SOL_KEY_W] = ACTION_FWD,      [SOL_KEY_A] = ACTION_LEFT,
    [SOL_KEY_S] = ACTION_BWD,       [SOL_KEY_D] = ACTION_RIGHT,    [SOL_KEY_F] = 0,
    [SOL_KEY_SPACE] = ACTION_JUMP,  [SOL_KEY_ESCAPE] = 0,          [SOL_KEY_SHIFT] = ACTION_DASH,
    [SOL_KEY_CTRL] = ACTION_CROUCH,
};

static void  Sol_Controller_Tick(World *world, double dt, double time);
static void  RemoteTick(World *world, int id, CompController *controller, double dt, double time);
static void  LocalTick(World *world, int id, double dt, double time);
static vec3s CalcWishdir3(uint32_t action, vec3s lookdir, vec3s updir);
static vec2s GetWishDir2(uint32_t action);

void Sol_Controller_Init(World *world)
{
    world->tickSystems[world->tickCount++] = Sol_Controller_Tick;

    world->controllers = calloc(MAX_ENTS, sizeof(CompController));
}

void Sol_Controller_Add(World *world, int id, ControllerKind kind)
{
    CompController controller = {
        .kind = kind,
    };
    world->masks[id] |= HAS_CONTROLLER;
    world->controllers[id] = controller;

    if (kind == CONTROLLER_LOCAL)
        world->playerID = id;
}

static void Sol_Controller_Tick(World *world, double dt, double time)
{
    int required = HAS_ACTIVE | HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompController *controller = &world->controllers[id];
        if (controller->kind == CONTROLLER_REMOTE)
            RemoteTick(world, id, controller, dt, time);
        if (controller->kind == CONTROLLER_LOCAL)
            LocalTick(world, id, dt, time);

        controller->aimpos = Sol_Physx_GetHeadPos(world, id);

        if (controller->isStrafing)
            world->xforms[id].quat = Sol_Quat_FromYawPitch(controller->yaw, 0);
        else if (glms_vec3_norm(controller->wishdir) > 0.001f)
        {
            float   target_entity_yaw = atan2f(controller->wishdir.x, controller->wishdir.z);
            versors target_quat       = Sol_Quat_FromYawPitch(target_entity_yaw, 0);

            // Smoothly turn the model toward the movement direction
            float turn_speed = 10.0f; // Higher numbers = faster turns
            float factor     = 1.0f - expf(-turn_speed * (float)dt);

            world->xforms[id].quat = glms_quat_slerp(world->xforms[id].quat, target_quat, factor);
        }
    }
}

static void RemoteTick(World *world, int id, CompController *controller, double dt, double time)
{
    // world->xforms[id].quat = Sol_Quat_FromYawPitch(controller->yaw, 0);
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

    SolMouse mouse         = Sol_Input_GetMouse();
    controller->isStrafing = mouse.locked;

    if (mouse.togglelocked)
    {
        if (mouse.buttons[SOL_MOUSE_LEFT])
            controller->actionState |= ACTION_ABILITY1;

        if (mouse.buttons[SOL_MOUSE_RIGHT])
            controller->actionState |= ACTION_ABILITY2;
    }
    else if (mouse.locked && mouse.buttons[SOL_MOUSE_LEFT])
        controller->actionState |= ACTION_FWD;

    controller->yaw     = look->yaw;
    controller->pitch   = look->pitch;
    controller->lookdir = look->lookdir;
    controller->wishdir = CalcWishdir3(controller->actionState, look->lookdir, WORLD_UP);

    controller->aimHitEnt = -1;
    SolRayResult aimTrace = Sol_Raycast(world, (SolRay){
                                                   .pos       = Sol_GetCamera()->pos,
                                                   .ignoreEnt = id,
                                                   .mask      = 0b01,
                                                   .dir       = look->lookdir,
                                                   .dist      = 60.f,
                                               });

    vec3s dir             = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, controller->aimpos));
    controller->aimdir    = vecDot(dir, look->lookdir) > 0.7f ? dir : look->lookdir;
    controller->aimHitEnt = aimTrace.entId;

    // #######################
    // #### DEBUG ACTIONS ####
    // #######################
    if (Sol_Input_KeyDown(SOL_KEY_F))
    {
        vec3s pos = glms_vec3_add(Sol_Xform_GetPos(world, id),
                                  glms_vec3_scale(Sol_Input_GetLook()->lookdir, (float)dt * 60.0f));
        Sol_Xform_Teleport(world, id, pos);
        Sol_Physx_SetVel(world, id, (vec3s){0, 0, 0});
    }

    // float freq = (GLM_PI_2f + look->pitch) * 200.0f;

    // Sol_Audio_SineFreq(0, freq);
    // Sol_Audio_SineFreq(1, freq);
    // Sol_Audio_SineFreq(2, freq);
    // Sol_Audio_SineFreq(3, freq);

    // if (Sol_Input_KeyDown(SOL_KEY_2))
    //     Sol_Audio_SetVolumeSine(0, 0.5f);
    // else
    //     Sol_Audio_SetVolumeSine(0, 0);
    // if (Sol_Input_KeyDown(SOL_KEY_3))
    //     Sol_Audio_SetVolumeSine(1, 0.5f);
    // else
    //     Sol_Audio_SetVolumeSine(1, 0);
    // if (Sol_Input_KeyDown(SOL_KEY_4))
    //     Sol_Audio_SetVolumeSine(2, 0.5f);
    // else
    //     Sol_Audio_SetVolumeSine(2, 0);
    // if (Sol_Input_KeyDown(SOL_KEY_5))
    //     Sol_Audio_SetVolumeSine(3, 0.5f);
    // else
    //     Sol_Audio_SetVolumeSine(3, 0);
}

static vec3s CalcWishdir3(uint32_t action, vec3s lookdir, vec3s updir)
{
    vec3s wishdir  = {0, 0, 0};
    vec3s flatdir  = lookdir;
    flatdir.y      = 0;
    flatdir        = glms_vec3_normalize(flatdir);
    vec3s rightdir = glms_vec3_normalize(glms_vec3_cross(flatdir, updir));
    if (action & ACTION_FWD)
        wishdir = glms_vec3_add(wishdir, flatdir);

    if (action & ACTION_BWD)
        wishdir = glms_vec3_sub(wishdir, flatdir);

    if (action & ACTION_RIGHT)
        wishdir = glms_vec3_add(wishdir, rightdir);

    if (action & ACTION_LEFT)
        wishdir = glms_vec3_sub(wishdir, rightdir);

    // if (action & ACTION_JUMP)
    //     wishdir = glms_vec3_add(wishdir, updir);

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

bool Sol_Controller_IsActionState(World *world, int id, SolActions mask)
{
    return (world->controllers[id].actionState & mask) != 0;
}

SolActions Sol_Controller_GetActionState(World *world, int id)
{
    return world->controllers[id].actionState;
}

vec3s Sol_Controller_GetAimPos(World *world, int id)
{
    CompController *controller = &world->controllers[id];
    return vecAdd(controller->aimpos, vecSca(controller->aimdir, 0.5f));
}

vec3s Sol_Controller_GetWishdir(World *world, int id)
{
    return world->controllers[id].wishdir;
}

SolActions Sol_GetActions(World *world, int id)
{
    return world->controllers[id].actionState;
}

vec3s Sol_GetWishdir(World *world, int id)
{
    return world->controllers[id].wishdir;
}

vec3s Sol_GetWishdir2(World *world, int id)
{
    return world->controllers[id].wishdir;
}

vec3s Sol_GetLookdir(World *world, int id)
{
    return world->controllers[id].lookdir;
}

vec3s Sol_GetAimpos(World *world, int id)
{
    return world->controllers[id].aimpos;
}

vec3s Sol_Controller_GetAimdir(World *world, int id)
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

vec3s Sol_Controller_GetShootPos(World *world, int id, float offset)
{
    vec3s head = world->controllers[id].aimpos;
    vec3s dir  = world->controllers[id].aimdir;
    return vecAdd(head, vecSca(dir, offset));
}

SolShoot Sol_Controller_GetShoot(World *world, int id, float speed)
{
    vec3s pos = Sol_Controller_GetShootPos(world, id, 0.5f);
    vec3s vel = vecSca(Sol_Controller_GetAimdir(world, id), speed);
    return (SolShoot){.pos = pos, .vel = vel};
}
