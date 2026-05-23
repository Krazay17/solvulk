#include "sol_core.h"

#include "movement_i.h"

#define DASH_VEL 14.0f
#define DASH_DURATION 0.25f
#define DAMPING 5.5f

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

    // Isolate the velocity strictly along the walljump direction
    float speedInJumpDir = glms_vec3_dot(vel, data->dir);

    if (speedInJumpDir > 0.0f)
    {
        // Calculate a gentle decay factor. As alpha goes down, damping softens.
        float alpha = 1.0f - (*elapsed / DASH_DURATION);

        // Bleed off velocity along the jump vector over time
        float speedLoss = speedInJumpDir * DAMPING * alpha * dt;

        // Apply the reduction along the jump track
        vel = glms_vec3_sub(vel, glms_vec3_scale(data->dir, speedLoss));
    }

    // Set the smoothly modified velocity back
    Sol_Physx_SetVel(world, id, vel);
}

void Walljump_State_Enter(World *world, int id)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];
    vec3s          dir = data->dir = glms_vec3_normalize(glms_vec3_lerp(data->surfaceNormal, WORLD_UP, 0.2f));

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
