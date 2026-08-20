/*
 * File: state_wallrun.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-17
 *
 */

#include "movement/si_movement.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "ability/si_ability.h"
#include "controller/s_controller.h"
#include "combat/s_combat.h"

#define MIN_WALL_ANGLE -0.7f
#define MAX_WALL_ANGLE 0.7f
#define COYOTE_TIMER 0.15f
#define BOOST_TIMEOUT 2.0f
#define BOOST_AMOUNT 9.0f
#define DISTANCE_CHECK 0.15f

static bool CheckEnergy(World *world, int id)
{
    if (!WHasB(world, id, HAS_COMBAT))
        return true;
    CompCombat *combat = &world->combats[id];
    if (combat->energy < 5.0f)
        return false;
    return true;
}

static bool CheckWall(World *world, int id, SolRayResult *result, float addRadius)
{
    CompXform *xform  = &world->xforms[id];
    vec3s      pos    = xform->pos;
    vec3s      dims   = Sol_Physx_GetDims(world, id);
    float      radius = dims.x + addRadius;
    for (int i = -1; i < 2; i++)
    {
        for (int j = 1; j < 9; j++)
        {
            vec3s finalPos = pos;
            finalPos.y += (float)i * (dims.y * 0.4f);
            vec3s rotated_offset = glms_quat_rotatev(xform->quat, VECTOR_RADIAL_DIRECTIONS[j]);
            *result       = Sol_RaycastD(world, (SolRay){.dist = radius, .dir = rotated_offset, .pos = finalPos}, 0.1f);
            float dot     = glms_vec3_dot(result->norm, WORLD_UP);
            vec3s lookDir = Sol_Controller_Get(world, id)->lookdir;
            float lookDot = vecDot(lookDir, result->norm);
            if (result->hit && dot > MIN_WALL_ANGLE && dot < MAX_WALL_ANGLE && lookDot < 0.6f)
            {
                CompMovement  *move         = &world->movements[id];
                MoveStateData *data         = &move->stateData[MOVE_WALLRUN];
                move->lastTouch             = result->pos;
                data->as.wallrun.wallNormal = result->norm;
                return true;
            }
        }
    }

    return false;
}

static bool LeaveState(World *world, int id)
{
    CompMovement *move = &world->movements[id];
    if (!CheckEnergy(world, id))
        return true;
    if (Sol_Controller_Get(world, id)->actionState & ACTION_CROUCH || Sol_Movement_GetGroundtime(world, id) > COYOTE_TIMER)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (!(Sol_Controller_Get(world, id)->actionState & ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_WALLJUMP))
            return true;
    if (Sol_Movement_SetState(world, id, MOVE_MANTLE))
        return true;
    return false;
}

void RunVel(World *world, int id, float boost)
{
    CompMovement  *movement   = &world->movements[id];
    MoveStateData *data       = &movement->stateData[MOVE_WALLRUN];
    vec3s          prevvel    = Sol_Physx_GetVel(world, id);
    vec3s          prevLatVel = prevvel;
    prevLatVel.y              = 0;
    float targetSpeed         = fmaxf(glms_vec3_norm(prevLatVel), boost);

    vec3s project;
    vec3s targetVel;
    vec3s lookdir      = Sol_Controller_Get(world, id)->lookdir;
    vec3s wishdir      = Sol_Controller_Get(world, id)->wishdir;
    lookdir.y          = 0;
    lookdir            = vecNorm(lookdir);
    float lookIntoWall = -glms_vec3_dot(lookdir, data->as.wallrun.wallNormal);

    if (lookIntoWall > 0.7f)
    {
        project   = glms_vec3_sub(lookdir, glms_vec3_scale(data->as.wallrun.wallNormal, -lookIntoWall));
        project.y = lookIntoWall;
        project   = glms_vec3_normalize(project);
        targetVel = glms_vec3_scale(project, targetSpeed);
    }
    else
    {
        float push_into_wall = glms_vec3_dot(prevLatVel, data->as.wallrun.wallNormal);
        project              = glms_vec3_sub(prevLatVel, glms_vec3_scale(data->as.wallrun.wallNormal, push_into_wall));
        project              = glms_vec3_normalize(project);
        targetVel            = glms_vec3_scale(project, targetSpeed);
        targetVel.y          = prevvel.y;
    }

    Sol_Physx_SetVel(world, id, targetVel);
}

void Wallrun_State_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;

    CompXform     *xform    = &world->xforms[id];
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[MOVE_WALLRUN];
    data->accum += dt;
    if (WHasB(world, id, HAS_COMBAT))
    {
        CompCombat *combat = &world->combats[id];
        combat->energy -= 5.0f * dt;
    }
    SolRayResult result   = {0};
    bool         goodWall = CheckWall(world, id, &result, DISTANCE_CHECK + 0.1f);
    if (goodWall)
    {
        data->accum = 0;
    }
    else if (!goodWall && data->accum >= COYOTE_TIMER)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }

    RunVel(world, id, Sol_Math_Lerp(BOOST_AMOUNT, 0.0f, data->elapsed / BOOST_TIMEOUT));

    vec3s dirToWall                    = glms_vec3_sub(xform->pos, movement->lastTouch);
    dirToWall                          = glms_vec3_normalize(dirToWall);
    float                 velToWallDot = -vecDot(Sol_Physx_GetVel(world, id), vecCrs(dirToWall, WORLD_UP));
    float                 speedDif     = 1.0f;
    const MoveStateForce *forces       = &MOVE_STATE_FORCES[movement->kind][movement->state];

    
    // ANIMATION
    float    x        = dirToWall.x;
    float    z        = dirToWall.z;
    vec3s    rot      = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc     = {.anim = ANIM_WALLRUN_FWD, .layerId = ANIM_LAYER_BASE};
    float    speedMod = 1.0f;
    switch (Sol_GetStrafedir(x, z, rot.x, rot.z))
    {
    case STRAFE_FWD:
        desc.anim = ANIM_WALK_FWD;
        break;
    case STRAFE_BWD:
        desc.anim = ANIM_WALLRUN_FWD;
        speedDif  = Sol_Physx_GetSpeed(world, id) / forces->speed;
        break;
    case STRAFE_LEFT:
    case STRAFE_FWD_LEFT:
    case STRAFE_BWD_LEFT:
        desc.anim = ANIM_WALLRUN_RIGHT;
        speedDif  = Sol_Physx_GetLatSpeed(world, id) / forces->speed;
        speedMod  = velToWallDot < 0 ? 1.5f : -1.5f;
        break;
    case STRAFE_RIGHT:
    case STRAFE_BWD_RIGHT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_WALLRUN_LEFT;
        speedDif  = Sol_Physx_GetLatSpeed(world, id) / forces->speed;
        speedMod  = velToWallDot > 0 ? 1.5f : -1.5f;
        break;
    }
    
    Sol_Model_PlayAnim(world, id, desc);
    speedDif = speedDif > 0 ? speedDif : 0.01f;
    Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_BASE, speedDif * speedMod);
}

void Wallrun_State_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[MOVE_WALLRUN];
    if (Sol_Physx_GetVel(world, id).y < 0.0f)
        Sol_Physx_SetVelY(world, id, 0.0f);
    data->enterVel = Sol_Physx_GetVel(world, id);
    RunVel(world, id, BOOST_AMOUNT);
    data->accum = 0;
}

void Wallrun_State_Exit(World *world, int id)
{
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[MOVE_WALLRUN];

    data->lastExited = solState.gameTime;
}

bool Wallrun_State_CanExit(World *world, int id, u32 nextState)
{
    return nextState != MOVE_WALLRUN;
}

bool Wallrun_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    if (Sol_Controller_Get(world, id)->actionState & ACTION_CROUCH)
        return false;
    SolRayResult result   = {0};
    bool         goodWall = CheckWall(world, id, &result, DISTANCE_CHECK);
    if (goodWall)
    {
        if (Sol_Ability_GetState(world, id) == ABILITY_STATE_DASH)
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true);
    }

    return goodWall;
}
