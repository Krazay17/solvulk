#include "sol_core.h"

#include "movement_i.h"

#define DURATION 0.1f

void Walljump_State_Update(World *world, int id, float dt)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];
    data->elapsed += dt;

    if (data->elapsed >= DURATION)
        Sol_Movement_SetState(world, id, MOVE_IDLE);
}

void Walljump_State_Enter(World *world, int id)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_WALLRUN];
    vec3s          dir  = glms_vec3_normalize(glms_vec3_lerp(data->surfaceNormal, WORLD_UP, 0.35f));
    Sol_Physx_AddVel(world, id, vecSca(dir, 6.66f));
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
