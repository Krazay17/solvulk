/*
 * File: s_controller.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Local Tick reads inputs and sets action states.
 * Remote Tick sets action states directly.
 */
#include "si_controller.h"

#include "sol_core.h"
#include "sol_math.h"

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

typedef struct
{
    int             cnt, cap;
    int            *sparse, *dense;
    CompController *controllers;

    // int               remote_cnt, remote_cap;
    // int              *remote_sparse, *remote_dense;
    // RemoteController *remote_controllers;

    // int           ai_cnt, ai_cap;
    // int          *ai_sparse, *ai_dense;
    // AiController *ai_controllers;
} WorldControllers;

static void SubTick(World *world, double dt, double time)
{
    float fdt = (float)dt;

    WorldControllers *wc = world->dense_components[WORLD_SYS_CONTROLLER];
    // for (int i = 0; i < wc->user_cnt; i++)
    // {
    //     int             id             = wc->user_dense[i];
    //     UserController *userController = &wc->user_controllers[i];
    //     CompController *controller     = &wc->controllers[wc->sparse[id]];
    //     User_Tick(world, id, controller, userController, dt, time);
    // }
    // for (int i = 0; i < wc->remote_cnt; i++)
    // {
    //     int               id               = wc->remote_dense[i];
    //     RemoteController *remoteController = &wc->remote_controllers[i];
    //     CompController   *controller       = &wc->controllers[wc->sparse[id]];
    //     Remote_Tick(world, controller, remoteController, dt, time);
    // }
    // for (int i = 0; i < wc->ai_cnt; i++)
    // {
    //     int             id           = wc->ai_dense[i];
    //     AiController   *aiController = &wc->ai_controllers[i];
    //     CompController *controller   = &wc->controllers[wc->sparse[id]];
    //     Ai_Tick(world, controller, aiController, dt, time);
    // }
    for (int i = 0; i < wc->cnt; i++)
    {
        int             id         = wc->dense[i];
        CompController *controller = &wc->controllers[i];
        if (WHasB(world, id, HAS_BODY3))
        {
            controller->aimpos = Sol_Physx_GetHeadPos(world, id);
        }

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
    }
}

void Sol_Controller_Init(World *world)
{
    WorldControllers *world_controllers           = malloc(sizeof(WorldControllers));
    world->dense_components[WORLD_SYS_CONTROLLER] = world_controllers;

    world_controllers->cap         = MAX_ENTS;
    world_controllers->cnt         = 0;
    world_controllers->controllers = malloc(world_controllers->cap * sizeof(CompController));
    world_controllers->sparse      = malloc(MAX_ENTS * sizeof(int));
    world_controllers->dense       = malloc(world_controllers->cap * sizeof(int));
    memset(world_controllers->sparse, -1, MAX_ENTS * sizeof(int));

    // world_controllers->user_cap         = 1;
    // world_controllers->user_cnt         = 0;
    // world_controllers->user_controllers = malloc(world_controllers->user_cap * sizeof(UserController));
    // world_controllers->user_sparse      = malloc(MAX_ENTS * sizeof(int));
    // world_controllers->user_dense       = malloc(world_controllers->user_cap * sizeof(int));
    // memset(world_controllers->user_sparse, -1, MAX_ENTS * sizeof(int));

    // world_controllers->remote_cap         = 8;
    // world_controllers->remote_cnt         = 0;
    // world_controllers->remote_controllers = malloc(world_controllers->remote_cap * sizeof(RemoteController));
    // world_controllers->remote_sparse      = malloc(MAX_ENTS * sizeof(int));
    // world_controllers->remote_dense       = malloc(world_controllers->remote_cap * sizeof(int));
    // memset(world_controllers->remote_sparse, -1, MAX_ENTS * sizeof(int));

    // world_controllers->ai_cap         = 64;
    // world_controllers->ai_cnt         = 0;
    // world_controllers->ai_controllers = malloc(world_controllers->ai_cap * sizeof(AiController));
    // world_controllers->ai_sparse      = malloc(MAX_ENTS * sizeof(int));
    // world_controllers->ai_dense       = malloc(world_controllers->ai_cap * sizeof(int));
    // memset(world_controllers->ai_sparse, -1, MAX_ENTS * sizeof(int));

    WAddTick(world) = SubTick;
}

// void Sol_Controller_Add(World *world, int id, ControllerKind kind)
// {
//     if (!WHasSys(world, WORLD_SYS_CONTROLLER))
//         return;
//     if (world->masks[id] & BITC(HAS_CONTROLLER))
//         return;
//     world->controllers[id] = (CompController){0};
//     world->masks[id] |= BITC(HAS_CONTROLLER);

//     switch (kind)
//     {
//     case CONTROLLERKIND_PLAYER:
//         Sol_Movement_Add(world, id, MOVEMENTKIND_PLAYER);
//         world->playerId = id;
//         break;
//     case CONTROLLERKIND_SPECTATE:
//         Sol_Movement_Add(world, id, MOVEMENTKIND_SPECTATE);
//         Sol_Movement_ForceState(world, id, MOVE_FLY);
//         world->playerId = id;
//         break;
//     case CONTROLLERKIND_WIZARD:
//         Sol_Movement_Add(world, id, MOVEMENTKIND_WIZARD);
//         Sol_Ai_Add(world, id, AIKIND_WIZARD);
//         break;
//     }
// }

CompController *Sol_Controller_Add(World *world, int id)
{
    WorldControllers *wc = world->dense_components[WORLD_SYS_CONTROLLER];
    if (wc->sparse[id] != -1)
        return &wc->controllers[wc->sparse[id]];
    if (wc->cnt >= wc->cap)
    {
        wc->cap *= 2;
        wc->dense       = realloc(wc->dense, wc->cap * sizeof(int));
        wc->controllers = realloc(wc->controllers, wc->cap * sizeof(CompController));
    }
    int denseIdx               = wc->cnt++;
    wc->sparse[id]             = denseIdx;
    wc->dense[denseIdx]        = id;
    CompController *controller = &wc->controllers[denseIdx];
    memset(controller, 0, sizeof(CompController));
    WAddComp(world, id, HAS_CONTROLLER);
    return controller;
}

// CompController *Sol_Controller_AddUser(World *world, int id, int playerIdx)
// {
//     CompController *controller = Sol_Controller_AddBase(world, id);
//     controller->kind           = CONTROLLERKIND_USER;

//     WorldControllers *world_controllers = world->dense_components[WORLD_SYS_CONTROLLER];
//     if (world_controllers->user_sparse[id] != -1)
//         return controller;
//     if (world_controllers->user_cnt >= world_controllers->user_cap)
//     {
//         world_controllers->user_cap *= 2;
//         world_controllers->user_dense =
//             realloc(world_controllers->user_dense, world_controllers->user_cap * sizeof(int));
//         world_controllers->user_controllers =
//             realloc(world_controllers->user_controllers, world_controllers->user_cap * sizeof(UserController));
//     }
//     int denseIdx                            = world_controllers->user_cnt++;
//     world_controllers->user_sparse[id]      = denseIdx;
//     world_controllers->user_dense[denseIdx] = id;
//     UserController *userController          = &world_controllers->user_controllers[denseIdx];
//     memset(userController, 0, sizeof(UserController));
//     userController->playerIdx = playerIdx;
//     return controller;
// }

void Sol_Controller_Remove(World *world, int id)
{
    WorldControllers *wc = world->dense_components[WORLD_SYS_CONTROLLER];
    for (int i = 0; i < wc->cnt; i++)
    {
        if (wc->dense[i] == id)
        {
            wc->sparse[id]     = -1;
            wc->controllers[i] = wc->controllers[--wc->cnt];
        }
    }
}

CompController *Sol_Controller_Get(World *world, int id)
{
    WorldControllers *world_controllers = world->dense_components[WORLD_SYS_CONTROLLER];
    return &world_controllers->controllers[world_controllers->sparse[id]];
}

// UserController *Sol_Controller_GetUser(World *world, int id)
// {
//     WorldControllers *wc  = world->dense_components[WORLD_SYS_CONTROLLER];
//     int               idx = wc->user_sparse[id];
//     if (idx == -1)
//         return NULL;
//     return &wc->user_controllers[idx];
// }

void Sol_Controller_SetParallaxAim(World *world, int id, vec3s lookpos, vec3s lookdir, float range, float hitdepth)
{
    CompController *controller = Sol_Controller_Get(world, id);
    SolRayResult    aimTrace   = Sol_Raycast(world, (SolRay){
                                                        .pos       = lookpos,
                                                        .ignoreEnt = id,
                                                        .mask      = COLLISIONGROUP_PAWN | COLLISIONGROUP_WORLD,
                                                        .dir       = lookdir,
                                                        .dist      = range,
                                                    });
    aimTrace.pos               = vecAdd(aimTrace.pos, vecSca(lookdir, hitdepth));
    vec3s dirFromTrace         = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, controller->aimpos));
    controller->aimdir         = vecDot(dirFromTrace, lookdir) > 0.7f ? dirFromTrace : lookdir;
    controller->aimHitEnt      = aimTrace.entId > -1 ? aimTrace.entId : -1;
}

vec3s Sol_Controller_GetShootPos(World *world, int id, float offset)
{

    vec3s head = Sol_Controller_Get(world, id)->aimpos;
    vec3s dir  = Sol_Controller_Get(world, id)->aimdir;
    return vecAdd(head, vecSca(dir, offset));
}

SolShoot Sol_Controller_GetShoot(World *world, int id, float offset, float speed)
{
    vec3s pos = Sol_Controller_GetShootPos(world, id, offset);
    vec3s vel = vecSca(Sol_Controller_Get(world, id)->aimdir, speed);
    return (SolShoot){.pos = pos, .vel = vel};
}