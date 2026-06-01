#include "sol_core.h"

#include "movement_i.h"

#define DASH_VEL 12.0f
#define DASH_DURATION 0.1f
#define DAMPING 9.5f

void Walljump_State_Update(World *world, int id, float dt)
{
    CompMovement  *move    = &world->movements[id];
    MoveStateData *data    = &move->stateData[MOVE_WALLRUN];
    float         *elapsed = &data->elapsed;

    *elapsed += dt;

    if (*elapsed >= DASH_DURATION)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE); // Switch to fall or idle
        return;
    }

    // Get current physics velocity
    vec3s vel = Sol_Physx_GetVel(world, id);

    float alpha = 1.0f - (*elapsed / DASH_DURATION);
    vel         = Sol_Math_InterpDir(vel, data->dir, alpha, DAMPING, dt);

    // Set the smoothly modified velocity back
    Sol_Physx_SetVel(world, id, vel);
}

void Walljump_State_Enter(World *world, int id)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];
    vec3s          vel  = glms_vec3_normalize(Sol_Physx_GetVel(world, id));
    vel.y               = 0;
    vec3s dir           = glms_vec3_normalize(glms_vec3_lerp(move->wallNormal, WORLD_UP, 0.15f));
    dir = data->dir = glms_vec3_lerp(dir, vel, 0.1f);

    vec3s currentVel = Sol_Physx_GetVel(world, id);

    // 2. Project current velocity onto the jump direction to see how fast they are already tracking
    float currentSpeedInJumpDir = glms_vec3_dot(currentVel, dir);

    // 3. If they are slower than the burst speed, boost them to it.
    //    If they are already flying faster, preserve their speed!
    if (currentSpeedInJumpDir < DASH_VEL)
    {
        float speedToAdd = DASH_VEL - currentSpeedInJumpDir;
        currentVel       = glms_vec3_add(currentVel, glms_vec3_scale(dir, speedToAdd));
    }

    Sol_Physx_SetVel(world, id, currentVel);
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
