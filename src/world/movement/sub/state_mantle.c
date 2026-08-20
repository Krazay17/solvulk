/*
 * File: state_mantle.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-07-05
 *
 */

#include "movement/si_movement.h"
#include "world.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "ability/s_ability.h"

#define RAY_COUNT 12
#define MANTLE_TIME 0.6f

static bool CheckWall(World *world, int id)
{
    CompMovement   *move       = &world->movements[id];
    MoveStateData  *data       = &move->stateData[MOVE_MANTLE];
    CompBody       *body       = &world->bodies[id];
    CompController *controller = Sol_Controller_Get(world, id);
    float           height     = body->dims.y;
    float           width      = body->dims.x;
    vec3s           pos        = Sol_Xform_GetPos(world, id);
    vec3s           basePos    = vecAdd(pos, vecSca(WORLD_UP, height * 0.7f));

    u32   mantleSpace = 0;
    float dist        = 0;
    vec3s goodPos     = {0};
    if (Sol_Raycast(world, (SolRay){.pos = pos, .dir = WORLD_UP, .dist = height, .ignoreEnt = id}).hit)
        return false;
    // Trace top down to find ledge
    for (int i = 0; i < RAY_COUNT; i++)
    {
        float offset = (float)i * (height / ((float)RAY_COUNT * 0.8f));
        vec3s pos    = basePos;
        pos.y -= offset;
        SolRay       ray = {.pos = pos, .dist = body->dims.x * 3.0f, .dir = Sol_Vec3_FromYawPitch(controller->yaw, 0)};
        SolRayResult rayResult = Sol_RaycastD(world, ray, 0.1f);
        // No hit indicates there is space above
        if (!rayResult.hit)
        {
            dist    = offset;
            goodPos = rayResult.pos;
            mantleSpace++;
        }
        // Hit after no hit indicates there is floor to mantle
        else if (mantleSpace > 1) // && vecDot(rayResult.norm, WORLD_UP) < 0.4f
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
    if (!Sol_Controller_Get(world, id)->actionState & ACTION_JUMP)
        return true;
    if (Sol_Controller_Get(world, id)->actionState & ACTION_CROUCH)
        return true;
    if (data->as.mantle.closeEnough)
        return true;

    return false;
}

void Mantle_State_Update(World *world, int id, float dt)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_MANTLE];
    if (LeaveState(world, id, data))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;

    vec3s pos       = Sol_Xform_GetPos(world, id);
    vec3s targetPos = data->as.mantle.pos;
    targetPos.y += Sol_Physx_GetDims(world, id).y * 0.5f;
    if (pos.y < targetPos.y && CheckWall(world, id))
    {
        Sol_Physx_SetVelY(world, id, 8.0f);
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
    if (Sol_Physx_GetVel(world, id).y > 5.0f)
    {
        data->as.mantle.doRoll = 1;
        Sol_Model_PlayAnim(world, id,
                           (AnimDesc){
                               .anim     = ANIM_MANTLE_ROLL,
                               .playKind = ANIMPLAYKIND_ONESHOT,
                               .speed    = 1.4f,
                               .layerId  = ANIM_LAYER_OVERRIDE,
                               .blendIn  = 0.1f,
                           });
    }
    else
    {
        data->as.mantle.doRoll = 0;
        Sol_Model_PlayAnim(world, id,
                           (AnimDesc){
                               .anim     = ANIM_MANTLE,
                               .playKind = ANIMPLAYKIND_ONESHOT,
                               .layerId  = ANIM_LAYER_OVERRIDE,
                               .speed    = 2.3f,
                               .blendIn  = 0.1f,
                           });
    }
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
