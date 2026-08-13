/*
 * File: s_controller.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Local Tick reads inputs and sets action states.
 * Remote Tick sets action states directly.
 */
#include "s_controller.h"
#include "sol_core.h"
#include "sol_math.h"
#include "camera.h"
#include "input.h"
#include "world.h"
#include "xform/s_xform.h"
#include "sol_user.h"
#include "buff/s_buff.h"
#include "physx/s_body.h"
#include "physx/s_body2d.h"
#include "building/s_building.h"
#include "platform/platform.h"

static const SolActions key_binds[SOL_KEY_COUNT] = {
    [SOL_KEY_Q] = ACTION_ABILITY1, [SOL_KEY_E] = ACTION_ABILITY2,

    [SOL_KEY_1] = ACTION_ABILITY3, [SOL_KEY_2] = ACTION_ABILITY4,
    [SOL_KEY_3] = ACTION_ABILITY5, [SOL_KEY_4] = ACTION_ABILITY6,

    [SOL_KEY_5] = ACTION_ABILITY7, [SOL_KEY_6] = ACTION_ABILITY8,
    [SOL_KEY_7] = ACTION_ABILITY9, [SOL_KEY_W] = ACTION_FWD,
    [SOL_KEY_A] = ACTION_LEFT,     [SOL_KEY_S] = ACTION_BWD,
    [SOL_KEY_D] = ACTION_RIGHT,    [SOL_KEY_F] = 0,
    [SOL_KEY_SPACE] = ACTION_JUMP, [SOL_KEY_ESCAPE] = 0,
    [SOL_KEY_SHIFT] = ACTION_DASH, [SOL_KEY_CTRL] = ACTION_CROUCH,
};
static const SolActions mouse_binds[SOL_MOUSE_COUNT] = {
    [SOL_MOUSE_LEFT]  = ACTION_ABILITY1,
    [SOL_MOUSE_RIGHT] = ACTION_ABILITY2,
};

static int  tick_required = BITC(HAS_ACTIVE) | BITC(HAS_CONTROLLER);
static void Controller_Tick(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, tick_required))
            continue;

        CompController *controller = &world->controllers[id];
        controller->aimpos         = Sol_Physx_GetHeadPos(world, id);

        controller->aimdir = Sol_Vec3_FromYawPitch(controller->yaw, controller->pitch);

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

static void Local_Tick(World *world, double dt, double time)
{
    int             localId    = 1;
    CompController *controller = &world->controllers[localId];
    SolMouse        mouse      = Sol_Input_GetMouse();

    float *yaw   = &controller->yaw;
    float *pitch = &controller->pitch;

    if (mouse.locked)
    {
        *yaw -= (float)(mouse.dx * user_settings.look_sens);
        *pitch -= (float)(mouse.dy * user_settings.look_sens);

        *yaw = fmodf(*yaw, 2.0f * GLM_PIf);
        if (*yaw > GLM_PIf)
            *yaw -= 2.0f * GLM_PIf;
        else if (*yaw < -GLM_PIf)
            *yaw += 2.0f * GLM_PIf;

        *pitch = glm_clamp(*pitch, -MAX_PITCH, MAX_PITCH);
    }

    vec3s lookdir = vecNorm(Sol_Vec3_FromYawPitch(*yaw, *pitch));

    for (int i = 0; i < SOL_KEY_COUNT; i++)
    {
        if (Sol_Input_KeyDown(i))
            controller->actionState |= key_binds[i];
        else
            controller->actionState &= ~key_binds[i];
    }

    controller->isStrafing = mouse.locked;

    if (WHas(world, 1, BITC(HAS_BUILDING)))
    {
        if (mouse.buttons[SOL_MOUSE_LEFT])
            controller->actionState |= ACTION_BUILD;
    }
    else
    {
        if (mouse.togglelocked)
        {
            if (mouse.buttons[SOL_MOUSE_LEFT])
                controller->actionState |= mouse_binds[SOL_MOUSE_LEFT];

            if (mouse.buttons[SOL_MOUSE_RIGHT])
                controller->actionState |= mouse_binds[SOL_MOUSE_RIGHT];
        }
        else if (mouse.locked && mouse.buttons[SOL_MOUSE_LEFT])
            controller->actionState |= ACTION_FWD;

        if (mouse.wheelV)
        {
            float changeDist = (float)mouse.wheelV * 0.01f;
            Sol_Camera_AdjustDistance(&solCamera, changeDist);
        }
    }

    controller->lookdir = lookdir;
    controller->wishdir = CalcWishdir3(controller->actionState, lookdir, WORLD_UP);

    controller->aimHitEnt = -1;
    SolRayResult aimTrace = Sol_Raycast(world, (SolRay){
                                                   .pos       = Sol_Cam_GetPos(),
                                                   .ignoreEnt = localId,
                                                   .mask      = 0b01,
                                                   .dir       = lookdir,
                                                   .dist      = 60.f,
                                               });
    aimTrace.pos          = vecAdd(aimTrace.pos, vecSca(lookdir, 0.5f));
    controller->aimHitPos = aimTrace.pos;
    vec3s dir             = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, controller->aimpos));

    controller->aimdir = vecDot(dir, lookdir) > 0.7f ? dir : lookdir;
    // controller->yaw       = input_yaw;
    // controller->pitch     = input_pitch;
    controller->aimHitEnt = aimTrace.entId;

    // #### DEBUG ACTIONS ####
    if (Sol_Input_KeyDown(SOL_KEY_F))
    {
        vec3s pos = glms_vec3_add(Sol_Xform_GetPos(world, localId), glms_vec3_scale(lookdir, (float)dt * 60.0f));
        Sol_Xform_Teleport(world, localId, pos);
        Sol_Physx_SetVel(world, localId, (vec3s){0, 0, 0});
    }

    if (Sol_Input_KeyPressed(SOL_KEY_5))
    {
        vec3s pos = Sol_Xform_GetPos(world, localId);
        if (!Sol_Buff_HasBuff(world, localId, BUFFKIND_INVULN))
            Sol_Buff_AddEx(world, localId, localId, BUFFKIND_INVULN, 99999.9f, 0);
        else
            Sol_Buff_Remove(world, localId, BUFFKIND_INVULN);
    }
    if (Sol_Input_KeyPressed(SOL_KEY_6))
    {
    }
}

static void Ai_Tick(World *world, double dt, double time)
{
}

static void Remote_Tick(World *world, double dt, double time)
{
}

void Sol_Controller_Init(World *world)
{
    WAddTick(world) = Local_Tick;
    WAddTick(world) = Ai_Tick;
    WAddTick(world) = Remote_Tick;
    WAddTick(world) = Controller_Tick;

    world->controllers = calloc(MAX_ENTS, sizeof(CompController));
}

void Sol_Controller_Add(World *world, int id)
{
    if (world->masks[id] & BITC(HAS_CONTROLLER))
        return;
    world->controllers[id] = (CompController){0};
    world->masks[id] |= BITC(HAS_CONTROLLER);
}

CompControllerLocal *Sol_ControllerLocal_Add(World *world, int id)
{
    CompControllerLocal *controllerLocal = &world->controllerLocal[id];
    memset(controllerLocal, 0, sizeof(CompControllerLocal));
    return controllerLocal;
}

CompControllerRemote *Sol_ControllerRemote_Add(World *world, int id)
{
    CompControllerRemote *controllerRemote = &world->controllerRemote[id];
    memset(controllerRemote, 0, sizeof(CompControllerRemote));
    return controllerRemote;
}

CompControllerAi *Sol_ControllerAi_Add(World *world, int id)
{
    CompControllerAi *controllerAi = &world->controllerAi[id];
    memset(controllerAi, 0, sizeof(CompControllerAi));
    return controllerAi;
}

bool Sol_Controller_WantsMove(World *world, int id)
{
    return glms_vec3_norm2(world->controllers[id].wishdir) > 0.0f;
}

bool Sol_Controller_IsActionState(World *world, int id, SolActions mask)
{
    return (world->controllers[id].actionState & mask) != 0;
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
    vec3s pos = Sol_Controller_GetShootPos(world, id, 1.5f);
    vec3s vel = vecSca(Sol_Controller_GetAimdir(world, id), speed);
    return (SolShoot){.pos = pos, .vel = vel};
}