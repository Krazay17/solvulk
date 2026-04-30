#include "sol_core.h"
#include "movement.h"

void Sol_Movement_Fly_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    CompBody *body = &world->bodies[id];
    CompController *controller = &world->controllers[id];

    if (!(controller->actionState & ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
    if (controller->actionState & ACTION_DASH)
        if (Sol_Movement_SetState(world, id, MOVE_DASH))
            return;

    const MoveStateForce *forces = &MOVE_STATE_FORCES[movement->configId][movement->moveState];
    vec3s vel = body->vel;
    vec3s wishdir = controller->wishdir;
    vel = ApplyFriction3(wishdir, vel, forces->friction, dt);
    vel = ApplyAccel3(wishdir, vel, forces->speed, forces->accell, dt);
    vel = ApplyAccel3((vec3s){0, 1.0f, 0}, vel, forces->speed, forces->accell, dt);

    body->vel = vel;
}

void Sol_Movement_Fly_Enter(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, ANIM_FALL, 0);
}

void Sol_Movement_Fly_Exit(World *world, int id)
{
}

bool Sol_Movement_Fly_CanEnter(World *world, int id)
{
    return true;
}

bool Sol_Movement_Fly_CanExit(World *world, int id)
{
    return true;
}
