/*
 * File: state_mantle.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-07-05
 *
 */

#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

#define RAY_COUNT 10
#define MANTLE_TIME 0.5f
#define MANTLE_SPEED 5.0f

static bool CheckWall(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    ScBody3 *body  = Sol_Comp_Get(world, id, ScBody3);
    Xform xform = Xform_Get(world, id);
    MoveStateData *data    = &move->stateData[MOVE_MANTLE];
    vec3s          basePos = vecAdd(xform.pos, vecSca(WORLD_UP, body->dims.y * 0.7f));

    if (Sol_Raycast1(world, (SolRay){ .start = xform.pos, .dir = WORLD_UP, .dist = body->dims.y, .ignoreEnt = id },
                     NULL))
        return false;

    vec3s goodPos     = { 0 };
    u32   mantleSpace = 0;
    // Trace top down to find ledge
    for (int i = 0; i < RAY_COUNT; i++)
    {
        float offset = (float)i * (body->dims.y / ((float)RAY_COUNT * 1.1f));
        vec3s pos    = basePos;
        pos.y -= offset;
        SolRay ray = {
            .start = pos, .dist = body->dims.x * 1.5f, .ignoreEnt = id, .dir = Sol_Vec3_FromYawPitch(cmd->yaw, 0)
        };
        SolRayResult rayResult;
        bool         hit = Sol_Raycast1(world, ray, &rayResult);
        // No hit indicates there is space above
        if (!hit)
        {
            goodPos = vecAdd(ray.start, vecSca(ray.dir, ray.dist));
            goodPos.y += body->dims.y * 0.5f;
            mantleSpace++;
        }
        // Hit after no hit indicates there is floor to mantle
        else if (mantleSpace > 2)
        {
            data->as.mantle.ledge_pos = rayResult.pos;
            data->as.mantle.pos       = goodPos;
            return true;
        }
    }
    return false;
}

void Move_Mantle_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    ScBody3       *body  = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data  = &move->stateData[MOVE_MANTLE];

    vec3s pos       = world->xform.pos[id];
    vec3s targetPos = data->as.mantle.pos;
    float speed     = MANTLE_SPEED;
    if (data->as.mantle.doRoll)
    {
        targetPos = vecAdd(targetPos, vecSca(Sol_Vec3_FromYawPitch(cmd->yaw, 0), 1.0f));
        speed += 2.0f;
    }
    if (pos.y < targetPos.y)
    {
        body->vel.y = speed;
    }
    else
    {
        vec3s dir            = vecSub(targetPos, pos);
        float dist           = glms_vec3_norm(dir);
        data->as.mantle.dist = dist;
        if (dist <= 0.2f && !CheckWall(world, id, move, cmd))
            data->as.mantle.closeEnough = 1;
        dir       = vecNorm(dir);
        body->vel = vecSca(dir, speed);
    }
}

void Move_Mantle_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    ScBody3       *body         = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data         = &move->stateData[MOVE_MANTLE];
    move->wantsJump             = false;
    data->as.mantle.closeEnough = 0;
    data->as.mantle.doRoll      = (body->vel.y > 5.0f) && (world->xform.pos[id].y < data->as.mantle.ledge_pos.y);
}

void Move_Mantle_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

bool Move_Mantle_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return move->stateData[move->state].as.mantle.closeEnough || !(cmd->actionState & BITC(ACTION_JUMP)) || move->stateData[move->state].elapsed >= MANTLE_TIME;
}

bool Move_Mantle_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (cmd->actionState & BITC(ACTION_JUMP))
        return CheckWall(world, id, move, cmd);
    return false;
}

void Move_Mantle_Draw(World *world, int id, ScMove3 *move, ScCmd *cmd, double dt)
{
}
