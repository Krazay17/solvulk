#include "sol_core.h"

void Sol_Movement_Fall_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    CompController *controller = &world->controllers[id];
    CompBody *body = &world->bodies[id];
    const MoveStateForce *forces = &MOVE_STATE_FORCES[movement->configId][movement->moveState];

    vec3s vel = body->vel;
    vec3s wishdir = controller->wishdir;
    float prevY = vel.y;

    if (body->grounded)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
    if (controller->actionState & ACTION_DASH)
        if (Sol_Movement_SetState(world, id, MOVE_DASH))
            return;
    // if (controller->actionState & ACTION_JUMP)
    //     if (Sol_Movement_SetState(world, id, MOVE_FLY))
    //         return;

    vec3s latwishdir = wishdir;
    latwishdir.y = 0;
    latwishdir = glms_vec3_normalize(latwishdir);

    vel = ApplyFriction3(latwishdir, vel, forces->friction, dt);
    vel = ApplyAccel3(latwishdir, vel, forces->speed, forces->accell, dt);
    vel.y = prevY;
    
    body->vel = vel;
}

void Sol_Movement_Fall_Enter(World *world, int id)
{
}

void Sol_Movement_Fall_Exit(World *world, int id)
{
}

bool Sol_Movement_Fall_CanEnter(World *world, int id)
{
    return true;
}

bool Sol_Movement_Fall_CanExit(World *world, int id)
{
    return true;
}
