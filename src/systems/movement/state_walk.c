#include "sol_core.h"
#include <cglm/struct.h>

void Sol_Movement_Walk_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    CompController *controller = &world->controllers[id];
    CompBody *body = &world->bodies[id];
    const MoveStateForce *forces = MOVE_STATE_FORCES[movement->moveState];

    vec3s vel = body->vel;
    vec3s wishdir = controller->wishdir;
    if (glms_vec3_norm(wishdir) == 0)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }
    if (controller->actionState & ACTION_JUMP)
    {
        Sol_Movement_SetState(world, id, MOVE_JUMP);
        return;
    }
    vec3s latwishdir = wishdir;
    latwishdir.y = 0;
    latwishdir = glms_vec3_normalize(latwishdir);
    vel = ApplyFriction3(latwishdir, vel, forces->friction, dt);
    vel = ApplyAccel3(latwishdir, vel, forces->speed, forces->accell, dt);
}

void Sol_Movement_Walk_Enter(World *world, int id)
{
    // todo
}

void Sol_Movement_Walk_Exit(World *world, int id)
{
    // todo
}