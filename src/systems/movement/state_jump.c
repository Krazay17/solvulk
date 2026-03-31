#include "sol_core.h"

void Sol_Movement_Jump_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    CompBody *body = &world->bodies[id];
    movement->stateTimer -= dt;
    if (movement->stateTimer <= 0)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
    body->vel.y = 7.0f;
}

void Sol_Movement_Jump_Enter(World *world, int id)
{
    CompMovement *movement = &world->movements[id];
    movement->stateTimer = 0.1f;
}

void Sol_Movement_Jump_Exit(World *world, int id)
{
}

bool Sol_Movement_Jump_CanEnter(World *world, int id)
{
    CompBody *body = &world->bodies[id];
    
    return body->grounded;
}

bool Sol_Movement_Jump_CanExit(World *world, int id)
{
    return true;
}
