#include "movement.h"
#include "sol_core.h"

void Sol_Movement_Walk_Update(World *world, int id, float dt)
{
    CompMovement         *movement   = &world->movements[id];
    CompController       *controller = &world->controllers[id];
    CompBody             *body       = &world->bodies[id];
    const MoveStateForce *forces     = &MOVE_STATE_FORCES[movement->configId][movement->moveState];

    vec3s vel     = body->vel;
    vec3s wishdir = controller->wishdir;

    if (controller->actionState & ACTION_JUMP)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return;
    if (!body->grounded)
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return;
    if (glms_vec3_norm(wishdir) == 0)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
    if (controller->actionState & ACTION_DASH)
        if (Sol_Movement_SetState(world, id, MOVE_DASH))
            return;

    vec3s latwishdir = wishdir;
    latwishdir.y     = 0;
    latwishdir       = glms_vec3_normalize(latwishdir);

    // Formula: v_projected = v - (v . normal) * normal
    float dot      = glms_vec3_dot(latwishdir, body->groundNormal);
    vec3s slopeDir = glms_vec3_sub(latwishdir, glms_vec3_scale(body->groundNormal, dot));

    // 3. Re-normalize!
    // Projecting onto an angle shortens the vector,
    // so we normalize to ensure we move at full speed.
    latwishdir = glms_vec3_normalize(slopeDir);

    vel = ApplyFriction3(latwishdir, vel, forces->friction, dt);
    vel = ApplyAccel3(latwishdir, vel, forces->speed, forces->accell, dt);

    body->vel = vel;
}

void Sol_Movement_Walk_Enter(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, ANIM_WALK_FWD, 0);
}

void Sol_Movement_Walk_Exit(World *world, int id)
{
}
bool Sol_Movement_Walk_CanEnter(World *world, int id)
{
    // todo
    return true;
}

bool Sol_Movement_Walk_CanExit(World *world, int id)
{
    // todo
    return true;
}
