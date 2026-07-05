/*
 * File: state_mantle.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-07-05
 *
 */

#include "movement_i.h"
#include "world.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"
#include "xform/s_xform.h"
#include "model/s_model.h"

#define RAY_COUNT 4
#define RAY_SPACING 0.1f
#define RAY_LENGTH_ADD 0.5f
#define MANTLE_TIME 0.5f

static bool CheckWall(World *world, int id)
{
    CompMovement   *move       = &world->movements[id];
    MoveStateData  *data       = &move->stateData[MOVE_MANTLE];
    CompBody       *body       = &world->bodies[id];
    CompController *controller = &world->controllers[id];
    vec3s           pos        = vecAdd(Sol_Xform_GetPos(world, id), vecSca(WORLD_UP, body->dims.y * 0.5f));

    bool doMantle = false;

    for (int i = 0; i < RAY_COUNT; i++)
    {
        float offset = (float)i * RAY_SPACING;
        pos.y -= offset;
        SolRay ray = {
            .pos = pos, .dist = body->dims.x + RAY_LENGTH_ADD, .dir = Sol_Vec3_FromYawPitch(controller->yaw, 0)};
        SolRayResult rayResult = Sol_Raycast(world, ray);
        if (rayResult.hit)
        {
            if (i == 0)
                return false;
            else
            {
                data->as.mantle.dist = offset;
                data->as.mantle.pos  = rayResult.pos;
                return true;
            }
        }
    }
    return false;
}

static bool LeaveState(World *world, int id, MoveStateData *data)
{
    if (data->elapsed >= MANTLE_TIME)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (!Sol_Controller_IsActionState(world, id, ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;

    return false;
}

void Mantle_State_Update(World *world, int id, float dt)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_MANTLE];
    data->elapsed += dt;
    if (LeaveState(world, id, data))
        return;

    vec3s pos       = Sol_Xform_GetPos(world, id);
    vec3s targetPos = data->as.mantle.pos;
    targetPos.y += Sol_Physx_GetDims(world, id).y * 0.5f;
    vec3s dir = vecSub(targetPos, pos);
    if (glms_vec3_norm(dir) <= 0.1f)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
    dir = vecNorm(dir);
    Sol_Physx_SetVel(world, id, vecSca(dir, 8.0f));

}

void Mantle_State_Enter(World *world, int id)
{
    CompMovement  *move     = &world->movements[id];
    MoveStateData *data     = &move->stateData[MOVE_MANTLE];
    move->wantsJump = false;
    Sol_Model_PlayAnim(world, id, (AnimDesc){.anim = ANIM_MANTLE, .playKind = ANIMPLAYKIND_ONESHOT, .speed = 2.5f});
}

void Mantle_State_Exit(World *world, int id)
{
}

bool Mantle_State_CanExit(World *world, int id, u32 nextState)
{
    return true;
}

bool Mantle_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    return CheckWall(world, id);
}

void Mantle_State_Draw(World *world, int id, double dt, double time)
{
}
