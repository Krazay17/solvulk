#include "movement_i.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

#define DASH_VEL 9.0f
#define DASH_DURATION 0.2f
#define DAMPING 6.0f

void Walljump_State_Update(World *world, int id, float dt)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];
    data->elapsed += dt;
    float alpha = 1.0f - (data->elapsed / DASH_DURATION);

    if (data->elapsed >= DASH_DURATION)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }

    vec3s vel = Sol_Physx_GetVel(world, id);
    vel       = Sol_Math_DampDir(vel, WORLD_UP, alpha, DAMPING, dt);
    //vel       = Sol_Math_DampDir(vel, data->dir, alpha, DAMPING, dt);
    Sol_Physx_SetVel(world, id, vel);
}

void Walljump_State_Enter(World *world, int id)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];
    vec3s          vel  = glms_vec3_normalize(Sol_Physx_GetVel(world, id));
    vel.y               = 0;
    vec3s finalDir      = vecAdd(move->wallNormal, WORLD_UP);
    finalDir            = vecAdd(finalDir, vel);
    data->dir           = vecNorm(finalDir);

    Sol_Physx_AddVel(world, id, vecSca(data->dir, DASH_VEL));
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
