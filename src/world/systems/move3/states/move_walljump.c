#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

#define DASH_VEL 12.0f
#define DASH_DURATION 0.5f
#define DAMPING 5.0f

void Move_Walljump_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    MoveStateData *walljumpData = &move->stateData[MOVE_WALLJUMP];

    if (walljumpData->elapsed >= DASH_DURATION)
    {
        Sol_Move3_SetState(world, id, MOVE_IDLE);
        return;
    }

    ScBody3 *body  = Sol_Comp_Get(world, id, ScBody3);
    float    alpha = 1.0f - (walljumpData->elapsed / DASH_DURATION);

    vec3s vel = body->vel;
    vel       = Sol_Math_DampDir(vel, WORLD_UP, alpha, DAMPING, dt);
    // vel       = Sol_Math_DampDir(vel, walljumpData->dir, alpha, DAMPING, dt);
    body->vel = vel;
}

void Move_Walljump_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    MoveStateData *wallrunData = &move->stateData[MOVE_WALLRUN];
    ScBody3       *body        = Sol_Comp_Get(world, id, ScBody3);

    vec3s vel      = body->vel;
    vec3s up2      = { 0.0f, 1.8f, 0.0f };
    vec3s finalDir = vecAdd(wallrunData->as.wallrun.wallNormal, up2);
    finalDir       = vecNorm(vecAdd(finalDir, vecNorm(vel)));

    vec3s finalVel    = vecSca(finalDir, DASH_VEL);
    float targetUpVel = finalVel.y;
    if (vel.y < targetUpVel)
        finalVel.y = targetUpVel - vel.y;
    else
        finalVel.y = 0;

    body->vel = vecAdd(body->vel, finalVel);
}

void Move_Walljump_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

bool Move_Walljump_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return move->stateData[move->state].elapsed > DASH_DURATION;
}

bool Move_Walljump_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (last == MOVE_WALLRUN)
    {
        return !(cmd->actionState & BITC(ACTION_JUMP));
    }
    return false;
}
