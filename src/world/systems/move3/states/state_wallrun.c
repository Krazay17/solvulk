/*
 * File: state_wallrun.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-17
 *
 */

#include "move3/s_move3.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"

#define MIN_WALL_ANGLE -0.7f
#define MAX_WALL_ANGLE 0.7f
#define COYOTE_TIMER 0.15f
#define BOOST_TIMEOUT 2.0f
#define BOOST_AMOUNT 9.0f
#define DISTANCE_CHECK 0.15f

static bool CheckEnergy(World *world, int id)
{
    if (!Sol_Comp_Has(world, id, ScCombat))
        return true;
    ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
       if (combat->energy < 5.0f)
        return false;
    return true;
}

static bool CheckWall(World *world, int id, SolRayResult *result, float addRadius)
{
    ScXform      *xform  = Sol_Comp_Get(world, id, ScXform);
    ScController *cont   = Sol_Comp_Get(world, id, ScController);
    ScMove3      *move   = Sol_Comp_Get(world, id, ScMove3);
    vec3s         dims   = {1, 1, 1}; // Sol_Physx_GetDims(world, id);
    float         radius = dims.x + addRadius;
    for (int i = -1; i < 2; i++)
    {
        for (int j = 1; j < 9; j++)
        {
            vec3s finalPos = xform->pos;
            finalPos.y += (float)i * (dims.y * 0.4f);
            vec3s rotated_offset = glms_quat_rotatev(xform->rot, VECTOR_RADIAL_DIRECTIONS[j]);
            bool  hit =
                Sol_Raycast1(world, (SolRay){.start = finalPos, .dist = radius, .dir = rotated_offset}, result);
            float dot     = glms_vec3_dot(result->norm, WORLD_UP);
            float lookDot = vecDot(cont->lookdir, result->norm);
            if (hit && dot > MIN_WALL_ANGLE && dot < MAX_WALL_ANGLE && lookDot < 0.6f)
            {
                MoveStateData *data         = &move->stateData[MOVE_WALLRUN];
                move->lastTouch             = result->pos;
                data->as.wallrun.wallNormal = result->norm;
                return true;
            }
        }
    }

    return false;
}

static bool LeaveState(World *world, int id, ScMove3 *move, ScController *cont)
{
    if (!CheckEnergy(world, id))
        return true;
    if (cont->actionState & BITC(ACTION_CROUCH) || move->groundtime > COYOTE_TIMER)
        if (Sol_Move3_SetState(world, id, MOVE_IDLE))
            return true;
    if (!(cont->actionState & BITC(ACTION_JUMP)))
        if (Sol_Move3_SetState(world, id, MOVE_WALLJUMP))
            return true;
    if (Sol_Move3_SetState(world, id, MOVE_MANTLE))
        return true;
    return false;
}

void RunVel(World *world, int id, float boost, ScMove3 *move, ScController *cont)
{
    ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);

    MoveStateData *data       = &move->stateData[MOVE_WALLRUN];
    vec3s          prevvel    = body->vel;
    vec3s          prevLatVel = prevvel;
    prevLatVel.y              = 0;
    float targetSpeed         = fmaxf(glms_vec3_norm(prevLatVel), boost);

    vec3s project;
    vec3s targetVel;
    vec3s lookdir      = cont->lookdir;
    vec3s wishdir      = cont->wishdir;
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

    body->vel = targetVel;
}

void Wallrun_State_Update(World *world, int id, float dt)
{
    ScMove3       *move  = Sol_Comp_Get(world, id, ScMove3);
    ScController  *cont  = Sol_Comp_Get(world, id, ScController);
    ScXform       *xform = Sol_Comp_Get(world, id, ScXform);
    MoveStateData *data  = &move->stateData[MOVE_WALLRUN];

    if (LeaveState(world, id, move, cont))
        return;

    data->accum += dt;

    SolRayResult result   = {0};
    bool         goodWall = CheckWall(world, id, &result, DISTANCE_CHECK + 0.1f);
    if (goodWall)
    {
        data->accum = 0;
    }
    else if (!goodWall && data->accum >= COYOTE_TIMER)
    {
        Sol_Move3_SetState(world, id, MOVE_IDLE);
        return;
    }

    RunVel(world, id, Sol_Math_Lerp(BOOST_AMOUNT, 0.0f, data->elapsed / BOOST_TIMEOUT), move, cont);

    vec3s dirToWall = glms_vec3_sub(xform->pos, move->lastTouch);
    dirToWall       = glms_vec3_normalize(dirToWall);
    // float                 velToWallDot = -vecDot(Sol_Physx_GetVel(world, id), vecCrs(dirToWall, WORLD_UP));
    // float                 speedDif     = 1.0f;
    // const MoveStateForce *forces       = &MOVE_STATE_FORCES[move->kind][move->state];

    data->as.wallrun.wallTouch = CalcTouch(data->as.wallrun.wallNormal, cont->yaw);
}

void Wallrun_State_Enter(World *world, int id)
{
    ScMove3       *move  = Sol_Comp_Get(world, id, ScMove3);
    ScController  *cont  = Sol_Comp_Get(world, id, ScController);
    ScXform       *xform = Sol_Comp_Get(world, id, ScXform);
    ScBody3       *body = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data  = &move->stateData[MOVE_WALLRUN];

    if (LeaveState(world, id, move, cont))
        return;
    if (body->vel.y < 0.0f)
        body->vel.y = 0.0f;
    RunVel(world, id, BOOST_AMOUNT, move, cont);
    data->accum = 0;
}

void Wallrun_State_Exit(World *world, int id)
{
    ScMove3       *move = Sol_Comp_Get(world, id, ScMove3);
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];
    data->lastExited    = solState.appTime;
}

bool Wallrun_State_CanExit(World *world, int id, u32 nextState)
{
    return nextState != MOVE_WALLRUN;
}

bool Wallrun_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    ScMove3      *move = Sol_Comp_Get(world, id, ScMove3);
    ScController *cont = Sol_Comp_Get(world, id, ScController);
    if (cont->actionState & BITC(ACTION_CROUCH))
        return false;
    SolRayResult result   = {0};
    bool         goodWall = CheckWall(world, id, &result, DISTANCE_CHECK);
    // if (goodWall)
    // {
    //     if (Sol_Ability_Get(world, id)->state == ABILITY_STATE_DASH)
    //         Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true);
    // }

    return goodWall;
}
