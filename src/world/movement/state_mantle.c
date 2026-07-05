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
#include "ability/s_ability.h"

#define RAY_COUNT 12
#define MANTLE_TIME 0.4f

static bool CheckWall(World *world, int id)
{
    CompMovement   *move       = &world->movements[id];
    MoveStateData  *data       = &move->stateData[MOVE_MANTLE];
    CompBody       *body       = &world->bodies[id];
    CompController *controller = &world->controllers[id];
    vec3s           basePos    = vecAdd(Sol_Xform_GetPos(world, id), vecSca(WORLD_UP, body->dims.y * 0.5f));

    bool  mantle  = false;
    float dist    = 0;
    vec3s goodPos = {0};
    for (int i = 0; i < RAY_COUNT; i++)
    {
        float offset = (float)i * (body->dims.y / ((float)RAY_COUNT * 1.33f));
        vec3s pos    = basePos;
        pos.y -= offset;
        SolRay       ray = {.pos = pos, .dist = body->dims.x * 2.4f, .dir = Sol_Vec3_FromYawPitch(controller->yaw, 0)};
        SolRayResult rayResult = Sol_Raycast(world, ray);
        if (!rayResult.hit)
        {
            dist    = offset;
            goodPos = rayResult.pos;
            mantle  = true;
        }
        else if (mantle == true && vecDot(rayResult.norm, WORLD_UP) < 0.4f)
        {
            data->as.mantle.dist = dist;
            data->as.mantle.pos  = goodPos;
            return true;
        }
    }
    return false;
}

static bool LeaveState(World *world, int id, MoveStateData *data)
{
    if (data->elapsed >= MANTLE_TIME)
        return true;
    if (!Sol_Controller_IsActionState(world, id, ACTION_JUMP))
        return true;
    if (Sol_Controller_IsActionState(world, id, ACTION_CROUCH))
        return true;
    if (data->as.mantle.closeEnough)
        return true;

    return false;
}

void Mantle_State_Update(World *world, int id, float dt)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_MANTLE];
    data->elapsed += dt;
    if (LeaveState(world, id, data))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;

    vec3s pos       = Sol_Xform_GetPos(world, id);
    vec3s targetPos = data->as.mantle.pos;
    targetPos.y += Sol_Physx_GetDims(world, id).y * 0.5f;
    if (pos.y < targetPos.y && CheckWall(world, id))
    {
        Sol_Physx_SetVelY(world, id, 6.0f);
    }
    else
    {
        vec3s dir = vecSub(targetPos, pos);
        if (glms_vec3_norm(dir) <= 0.15f)
            data->as.mantle.closeEnough = 1;
        dir = vecNorm(dir);
        Sol_Physx_SetVel(world, id, vecSca(dir, 8.0f));
    }
}

void Mantle_State_Enter(World *world, int id)
{
    CompMovement  *move         = &world->movements[id];
    MoveStateData *data         = &move->stateData[MOVE_MANTLE];
    move->wantsJump             = false;
    data->as.mantle.closeEnough = 0;
    Sol_Model_PlayAnim(world, id, (AnimDesc){.anim = ANIM_MANTLE, .playKind = ANIMPLAYKIND_ONESHOT, .speed = 2.3f});
}

void Mantle_State_Exit(World *world, int id)
{
}

bool Mantle_State_CanExit(World *world, int id, u32 nextState)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_MANTLE];
    return LeaveState(world, id, data);
}

bool Mantle_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    CompAbility *ability = &world->abilities[id];
    if (ability->state == ABILITY_STATE_DASH || ability->state == ABILITY_STATE_SPINSLASH)
        return false;
    return CheckWall(world, id);
}

void Mantle_State_Draw(World *world, int id, double dt, double time)
{
}
