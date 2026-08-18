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
#include "movement/s_movement.h"
#include "ai/s_ai.h"

static void Controller_Tick(World *world, double dt, double time)
{
    static int  required = BITC(HAS_ACTIVE) | BITC(HAS_CONTROLLER);

    float fdt = (float)dt;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, required))
            continue;

        CompController *controller = &world->controllers[id];

        controller->wishdir   = CalcWishdir3(controller->actionState, controller->lookdir, WORLD_UP);
        controller->wishdir2d = CalcWishDir2(controller->actionState);

        if (controller->isStrafing)
            world->xforms[id].quat = Sol_Quat_FromYawPitch(controller->yaw, 0);
        else if (glms_vec3_norm(controller->wishdir) > 0.001f)
        {
            float   target_entity_yaw = atan2f(controller->wishdir.x, controller->wishdir.z);
            versors target_quat       = Sol_Quat_FromYawPitch(target_entity_yaw, 0);

            // Smoothly turn the model toward the movement direction
            float turn_speed = 10.0f; // Higher numbers = faster turns
            float factor     = 1.0f - expf(-turn_speed * fdt);

            world->xforms[id].quat = glms_quat_slerp(world->xforms[id].quat, target_quat, factor);
        }

        if (WHasB(world, id, HAS_BODY3))
        {
            controller->aimpos = Sol_Physx_GetHeadPos(world, id);
            if (controller->actionState & ACTION_DEBUGTELE)
            {
                vec3s pos =
                    glms_vec3_add(Sol_Xform_GetPos(world, id), glms_vec3_scale(controller->lookdir, fdt * 60.0f));
                Sol_Xform_Teleport(world, id, pos);
                Sol_Physx_SetVel(world, id, (vec3s){0, 0, 0});
            }
        }
    }
}

void Sol_Controller_Init(World *world)
{
    world->controllers = calloc(MAX_ENTS, sizeof(CompController));

    WAddTick(world) = Controller_Tick;
}

void Sol_Controller_Add(World *world, int id, ControllerKind kind)
{
    if (!WHasSys(world, WORLD_SYS_CONTROLLER))
        return;
    if (world->masks[id] & BITC(HAS_CONTROLLER))
        return;
    world->controllers[id] = (CompController){0};
    world->masks[id] |= BITC(HAS_CONTROLLER);

    switch (kind)
    {
    case CONTROLLERKIND_PLAYER:
        Sol_Movement_Add(world, id, MOVEMENTKIND_PLAYER);
        world->playerID = id;
        break;
    case CONTROLLERKIND_WIZARD:
        Sol_Movement_Add(world, id, MOVEMENTKIND_WIZARD);
        Sol_Ai_Add(world, id, AIKIND_WIZARD);
        break;
    }
}

void Sol_Controller_SetParallaxAim(World *world, int id, vec3s lookpos, vec3s lookdir, float range, float hitdepth)
{
    CompController *controller = &world->controllers[id];
    SolRayResult    aimTrace   = Sol_Raycast(world, (SolRay){
                                                        .pos       = lookpos,
                                                        .ignoreEnt = id,
                                                        .mask      = COLLISIONGROUP_PAWN | COLLISIONGROUP_WORLD,
                                                        .dir       = lookdir,
                                                        .dist      = range,
                                                    });
    aimTrace.pos               = vecAdd(aimTrace.pos, vecSca(lookdir, hitdepth));

    controller->aimpos = aimTrace.pos;
    vec3s dirFromTrace = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, Sol_Physx_GetHeadPos(world, id)));

    controller->aimdir    = vecDot(dirFromTrace, controller->aimdir) > 0.7f ? dirFromTrace : lookdir;
    controller->aimHitEnt = aimTrace.entId > -1 ? aimTrace.entId : -1;
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