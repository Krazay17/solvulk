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

static bool CheckWall(World *world, int id, ScMove3 *move, SolRayResult *result, float addRadius)
{
    ScXform       *xform = Sol_Comp_Get(world, id, ScXform);
    MoveStateData *data  = &move->stateData[MOVE_WALLRUN];

    vec3s dims   = Sol_Comp_Get(world, id, ScBody3)->dims;
    float radius = dims.x + addRadius;

    for (int i = -1; i < 2; i++)
    {
        for (int j = 1; j < 9; j++)
        {
            vec3s finalPos = xform->pos;
            finalPos.y += (float)i * (dims.y * 0.4f);
            vec3s  rotated_offset = glms_quat_rotatev(xform->rot, VECTOR_RADIAL_DIRECTIONS[j]);
            SolRay ray            = {
                .start = finalPos, .dist = radius + 0.1f, .dir = rotated_offset, .ignoreEnt = id, .debug = true
            };
            bool  hit = Sol_Raycast1D(world, ray, result, 0.1f);
            float dot = glms_vec3_dot(result->norm, WORLD_UP);
            // float lookDot = vecDot(cmd->lookdir, result->norm);
            if (hit && dot > MIN_WALL_ANGLE && dot < MAX_WALL_ANGLE)
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

void RunVel(World *world, int id, float boost, ScMove3 *move, ScCmd *cmd)
{
    ScBody3       *body = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];

    vec3s prevvel     = body->vel;
    vec3s prevLatVel  = prevvel;
    prevLatVel.y      = 0;
    float targetSpeed = fmaxf(glms_vec3_norm(prevLatVel), boost);

    vec3s project;
    vec3s targetVel;
    vec3s lookdir      = cmd->lookdir;
    vec3s wishdir      = cmd->wishdir;
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

void Move_Wallrun_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    ScXform       *xform = Sol_Comp_Get(world, id, ScXform);
    MoveStateData *data  = &move->stateData[MOVE_WALLRUN];

    data->accum += dt;

    SolRayResult result   = { 0 };
    bool         goodWall = CheckWall(world, id, move, &result, DISTANCE_CHECK + 0.1f);
    if (goodWall)
    {
        data->accum = 0;
    }
    // else if (!goodWall && data->accum >= COYOTE_TIMER)
    // {
    //     Sol_Move3_SetState(world, id, MOVE_IDLE);
    //     return;
    // }

    RunVel(world, id, Sol_Math_Lerp(BOOST_AMOUNT, 0.0f, data->elapsed / BOOST_TIMEOUT), move, cmd);

    vec3s dirToWall            = glms_vec3_sub(xform->pos, move->lastTouch);
    dirToWall                  = glms_vec3_normalize(dirToWall);
    data->as.wallrun.wallTouch = CalcTouch(data->as.wallrun.wallNormal, cmd->yaw);
}

void Move_Wallrun_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    ScXform       *xform = Sol_Comp_Get(world, id, ScXform);
    ScBody3       *body  = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data  = &move->stateData[MOVE_WALLRUN];

    if (body->vel.y < 0.0f)
        body->vel.y = 0.0f;
    RunVel(world, id, BOOST_AMOUNT, move, cmd);
    data->accum = 0;
}

void Move_Wallrun_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];
    data->lastExited    = solState.appTime;
}

bool Move_Wallrun_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return true;
}

bool Move_Wallrun_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (cmd->actionState & BITC(ACTION_CROUCH))
        return false;

    if (move->airtime > 0 && (cmd->actionState & BITC(ACTION_JUMP)))
    {
        SolRayResult result   = { 0 };
        bool         goodWall = CheckWall(world, id, move, &result, DISTANCE_CHECK);
        return goodWall;
    }
    return false;
}
