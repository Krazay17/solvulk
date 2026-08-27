#include "movement/si_movement.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

#define DASH_VEL 13.0f
#define DASH_DURATION 0.45f
#define DAMPING 4.0f

void Walljump_State_Update(World *world, int id, float dt)
{
    CompMovement  *move         = &world->movements[id];
    MoveStateData *walljumpData = &move->stateData[MOVE_WALLJUMP];
    float          alpha        = 1.0f - (walljumpData->elapsed / DASH_DURATION);

    if (walljumpData->elapsed >= DASH_DURATION)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }

    vec3s vel = Sol_Physx_GetVel(world, id);
    vel       = Sol_Math_DampDir(vel, WORLD_UP, alpha, DAMPING, dt);
    // vel       = Sol_Math_DampDir(vel, walljumpData->dir, alpha, DAMPING, dt);
    Sol_Physx_SetVel(world, id, vel);
}

void Walljump_State_Enter(World *world, int id)
{
    CompMovement  *move        = &world->movements[id];
    MoveStateData *wallrunData = &move->stateData[MOVE_WALLRUN];
    vec3s          vel         = Sol_Physx_GetVel(world, id);
    vec3s          up2         = {0.0f, 1.8f, 0.0f};
    vec3s          finalDir    = vecAdd(wallrunData->as.wallrun.wallNormal, up2);
    finalDir                   = vecAdd(finalDir, vecNorm(vel));
    wallrunData->dir           = vecNorm(finalDir);

    vec3s finalVel    = vecSca(wallrunData->dir, DASH_VEL);
    float targetUpVel = finalVel.y;
    if (vel.y < targetUpVel)
        finalVel.y = targetUpVel - vel.y;
    else
        finalVel.y = 0;

    Sol_Physx_AddVel(world, id, finalVel);
}

void Walljump_State_Exit(World *world, int id)
{
    Sol_Model_StopAnim(world, id, ANIM_LAYER_BASE, 0.2f);
}

bool Walljump_State_CanExit(World *world, int id, u32 nextState)
{
    return nextState != MOVE_WALLJUMP;
}

bool Walljump_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    return true;
}
