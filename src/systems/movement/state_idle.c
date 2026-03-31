#include <cglm/struct.h>

#include "sol_core.h"

void Sol_Movement_Idle_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    CompController *controller = &world->controllers[id];
    CompBody *body = &world->bodies[id];
    const MoveStateForce *forces = &MOVE_STATE_FORCES[movement->configId][movement->moveState];

    vec3s vel = body->vel;
    vec3s wishdir = controller->wishdir;

    if (!body->grounded)
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return;
    if (glms_vec3_norm(wishdir) > 0)
        if (Sol_Movement_SetState(world, id, MOVE_WALK))
            return;

    vel = ApplyFriction3((vec3s){0, 0, 0}, vel, forces->friction, dt);

    body->vel = vel;
}

void Sol_Movement_Idle_Enter(World *world, int id)
{
    // todo
}

void Sol_Movement_Idle_Exit(World *world, int id)
{
    // todo
}

bool Sol_Movement_Idle_CanEnter(World *world, int id)
{
    // todo
    return true;
}

bool Sol_Movement_Idle_CanExit(World *world, int id)
{
    // todo
    return true;
}
