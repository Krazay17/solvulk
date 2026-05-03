#include "move_states.h"
#include "sol_core.h"

#define DASH_VEL 26.0f
#define DASH_ALPHAMOD 1.2f
#define DASH_DURATION 0.3f

void Sol_Movement_Dash_Update(World *world, int id, float dt)
{
    CompMovement         *movement   = &world->movements[id];
    CompController       *controller = &world->controllers[id];
    CompBody             *body       = &world->bodies[id];
    const MoveStateForce *forces     = &MOVE_STATE_FORCES[movement->configId][movement->moveState];

    movement->stateTimer += dt;
    if (movement->stateTimer >= DASH_DURATION)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;

    float alpha = DASH_ALPHAMOD - (movement->stateTimer / DASH_DURATION);

    vec3s vel = glms_vec3_scale(movement->lockdir, alpha * DASH_VEL);
    body->vel = vel;
}

void Sol_Movement_Dash_Enter(World *world, int id)
{
    CompMovement   *movement   = &world->movements[id];
    CompController *controller = &world->controllers[id];

    vec3s dashdir = Sol_Vec3_FromYawPitch(controller->yaw, 0);
    if (glms_vec3_norm(movement->wishdir) > 0 && vecDot(movement->wishdir, WORLD_UP) < 0.99f)
        dashdir = movement->wishdir;

    dashdir.y = 0;
    dashdir   = glms_vec3_normalize(dashdir);

    movement->lockdir    = dashdir;
    movement->stateTimer = 0;

    switch (Get_StrafeDir(dashdir.x, dashdir.z, controller->lookdir.x, controller->lookdir.z))
    {
    case STRAFE_FWD:
        Sol_Model_PlayAnim(world, id, ANIM_DASH_FWD, 0);
        break;
    case STRAFE_BWD:
        Sol_Model_PlayAnim(world, id, ANIM_DASH_BWD, 0);
        break;
    case STRAFE_LEFT:
        Sol_Model_PlayAnim(world, id, ANIM_DASH_LEFT, 0);
        break;
    case STRAFE_RIGHT:
        Sol_Model_PlayAnim(world, id, ANIM_DASH_RIGHT, 0);
        break;
    }
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
