#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

#define DASH_VEL 13.0f
#define DASH_DURATION 0.45f
#define DAMPING 4.0f

void Walljump_State_Update(World *world, int id, float dt)
{
    ScMove3       *move         = Sol_Comp_Get(world, id, ScMove3);
    MoveStateData *walljumpData = &move->stateData[MOVE_WALLJUMP];

    if (walljumpData->elapsed >= DASH_DURATION)
    {
        Sol_Move3_SetState(world, id, MOVE_IDLE);
        return;
    }

    ScCmd *cmd  = Sol_Comp_Get(world, id, ScCmd);
    ScBody3      *body  = Sol_Comp_Get(world, id, ScBody3);
    float         alpha = 1.0f - (walljumpData->elapsed / DASH_DURATION);

    vec3s vel = body->vel;
    vel       = Sol_Math_DampDir(vel, WORLD_UP, alpha, DAMPING, dt);
    // vel       = Sol_Math_DampDir(vel, walljumpData->dir, alpha, DAMPING, dt);
    body->vel = vel;
}

void Walljump_State_Enter(World *world, int id)
{
    ScMove3       *move        = Sol_Comp_Get(world, id, ScMove3);
    MoveStateData *wallrunData = &move->stateData[MOVE_WALLRUN];
    ScCmd  *cmd        = Sol_Comp_Get(world, id, ScCmd);
    ScBody3       *body        = Sol_Comp_Get(world, id, ScBody3);

    vec3s vel        = body->vel;
    vec3s up2        = {0.0f, 1.8f, 0.0f};
    vec3s finalDir   = vecAdd(wallrunData->as.wallrun.wallNormal, up2);
    finalDir         = vecAdd(finalDir, vecNorm(vel));
    wallrunData->dir = vecNorm(finalDir);

    vec3s finalVel    = vecSca(wallrunData->dir, DASH_VEL);
    float targetUpVel = finalVel.y;
    if (vel.y < targetUpVel)
        finalVel.y = targetUpVel - vel.y;
    else
        finalVel.y = 0;

    body->vel = vecAdd(body->vel, finalVel);
}

void Walljump_State_Exit(World *world, int id)
{
}

bool Walljump_State_CanExit(World *world, int id, u32 nextState)
{
    return nextState != MOVE_WALLJUMP;
}

bool Walljump_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    return true;
}
