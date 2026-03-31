#include <cglm/struct.h>

#include "sol_core.h"

void Sol_Movement_Dash_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    CompController *controller = &world->controllers[id];
    CompBody *body = &world->bodies[id];
    const MoveStateForce *forces = &MOVE_STATE_FORCES[movement->configId][movement->moveState];

    vec3s vel = body->vel;
    vec3s wishdir = controller->wishdir;
    movement->stateTimer -= dt;

    if(movement->stateTimer <= 0)
        if(Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;

    vel = ApplyAccel3(movement->lockdir, vel, forces->speed, forces->accell, dt);

    body->vel = vel;
}

void Sol_Movement_Dash_Enter(World *world, int id)
{
    CompMovement *movement = &world->movements[id];
    movement->stateTimer = 0.5f;
    movement->lockdir = movement->wishdir;
}

void Sol_Movement_Dash_Exit(World *world, int id)
{
}

bool Sol_Movement_Dash_CanEnter(World *world, int id)
{
    return true;
}

bool Sol_Movement_Dash_CanExit(World *world, int id)
{
    return true;
}
