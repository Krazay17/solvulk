#include "sol_core.h"

#include "movement_i.h"

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
    vel       = Sol_Math_InterpDir(vel, WORLD_UP, alpha, DAMPING, dt);
    Sol_Physx_SetVel(world, id, vel);
}

void Walljump_State_Enter(World *world, int id)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];
    vec3s          vel  = glms_vec3_normalize(Sol_Physx_GetVel(world, id));
    vel.y               = 0;
    vec3s dir           = glms_vec3_normalize(glms_vec3_lerp(move->wallNormal, WORLD_UP, 0.2f));
    dir = data->dir = glms_vec3_normalize(glms_vec3_lerp(dir, vel, 0.2f));

    // if (Sol_Physx_GetVel(world, id).y < 0)
    //     Sol_Physx_SetVelY(world, id, 0);
    Sol_Physx_AddVel(world, id, vecSca(dir, DASH_VEL));

    // vec3s currentVel = Sol_Physx_GetVel(world, id);
    // // 2. Project current velocity onto the jump direction to see how fast they are already tracking
    // float currentSpeedInJumpDir = glms_vec3_dot(currentVel, dir);

    // // 3. If they are slower than the burst speed, boost them to it.
    // //    If they are already flying faster, preserve their speed!
    // if (currentSpeedInJumpDir < DASH_VEL)
    // {
    //     float speedToAdd = DASH_VEL - currentSpeedInJumpDir;
    //     currentVel       = glms_vec3_add(currentVel, glms_vec3_scale(dir, speedToAdd));
    // }

    // Sol_Physx_SetVel(world, id, currentVel);
}

void Walljump_State_Exit(World *world, int id)
{
}

bool Walljump_State_CanExit(World *world, int id, u32 nextState)
{
    return nextState != MOVE_WALLJUMP;
}

bool Walljump_State_CanEnter(World *world, int id, u32 lastState, u32 nextState)
{
    return true;
}
