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

        SolRayResult aimTrace = Sol_Raycast(world, (SolRay){
                                                       .pos       = controller->aimpos,
                                                       .ignoreEnt = id,
                                                       .mask      = 0b01,
                                                       .dir       = controller->aimdir,
                                                       .dist      = 60.f,
                                                   });
        aimTrace.pos          = vecAdd(aimTrace.pos, vecSca(controller->aimdir, 0.5f));

        controller->aimHitPos = aimTrace.pos;
        vec3s dirFromTrace    = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, controller->aimpos));
        controller->aimdir    = vecDot(dirFromTrace, controller->aimdir) > 0.7f ? dirFromTrace : controller->aimdir;
        controller->aimHitEnt = aimTrace.entId > -1 ? aimTrace.entId : -1;

        controller->aimdir = Sol_Vec3_FromYawPitch(controller->yaw, controller->pitch);

        if (controller->actionState & ACTION_LEFT)
            controller->wishdir2d = GetWishDir2(ACTION_LEFT);
        else if (controller->actionState & ACTION_RIGHT)
            controller->wishdir2d = GetWishDir2(ACTION_RIGHT);
        controller->aimpos2d = Sol_Input_GetMouseUI();

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

static void Ai_Tick(World *world, double dt, double time)
{
}

static void Remote_Tick(World *world, double dt, double time)
{
}

void Sol_Controller_Init(World *world)
{
    world->controllers = calloc(MAX_ENTS, sizeof(CompController));

    WAddTick(world) = Ai_Tick;
    WAddTick(world) = Remote_Tick;
    WAddTick(world) = Controller_Tick;
}

void Sol_Controller_Add(World *world, int id)
{
    if (!WHasSys(world, WORLD_SYS_CONTROLLER))
        return;
    if (world->masks[id] & BITC(HAS_CONTROLLER))
        return;
    world->controllers[id] = (CompController){0};
    world->masks[id] |= BITC(HAS_CONTROLLER);
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