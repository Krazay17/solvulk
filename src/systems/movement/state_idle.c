#include "sol_movement.h"

void Sol_Movement_Idle_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    CompController *controller = &world->controllers[id];
    CompBody *body = &world->bodies[id];
    const MoveStateForce *forces = MOVE_STATE_FORCES[movement->moveState];

    vec3s vel = body->vel;
    vec3s wishdir = controller->wishdir;

    if (glms_vec3_norm(wishdir) > 0)
    {
        Sol_Movement_SetState(world, id, MOVE_WALK);
        return;
    }
    vel = ApplyFriction3((vec3s){0, 0, 0}, vel, forces->friction, dt);
}

void Sol_Movement_Idle_Enter(World *world, int id)
{
    // todo
}

void Sol_Movement_Idle_Exit(World *world, int id)
{
    // todo
}