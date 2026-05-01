#include "sol_core.h"
#include "movement.h"

CompMovement *Sol_Movement_Add(World *world, int id, CompMovement init)
{
    CompMovement movement = init;
    world->masks[id] |= HAS_MOVEMENT;
    return &world->movements[id];
}

void Sol_System_Movement_3d_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_MOVEMENT | HAS_XFORM | HAS_BODY3;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompMovement *movement = &world->movements[id];
            CompXform    *xform    = &world->xforms[id];
            CompBody     *body     = &world->bodies[id];

            CompController *controller = &world->controllers[id];
            if (controller)
                movement->wishdir = controller->wishdir;

            xform->quat = Sol_Quat_FromYawPitch(controller->yaw, 0); // -controller->pitch

            const MoveStateFunc *funcs = &MOVE_STATE_FUNCS[movement->configId][movement->moveState];
            funcs->update(world, id, dt);
        }
    }
    if (world->playerID < 0)
        return;

    float speed = glms_vec3_norm(world->bodies[world->playerID].vel);
    Sol_Debug_Add("Velocity", speed);
}

bool Sol_Movement_SetState(World *world, int id, MoveState nextState)
{
    CompMovement *movement = &world->movements[id];
    if (movement->moveState == nextState)
        return false;
    const MoveStateFunc *prevfunc = &MOVE_STATE_FUNCS[movement->configId][movement->moveState];
    if (!prevfunc->canExit(world, id))
        return false;
    const MoveStateFunc *nextfunc = &MOVE_STATE_FUNCS[movement->configId][nextState];
    if (!nextfunc->canEnter(world, id))
    return false;

    //printf("LastState: %d, CurrentState: %d\n", movement->moveState, nextState);

    prevfunc->exit(world, id);
    movement->moveState = nextState;
    nextfunc->enter(world, id);
    Sol_Debug_Add("state", movement->moveState);

    return true;
}

void Sol_System_Movement_2d_Step(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_MOVEMENT | HAS_XFORM | HAS_BODY2 | HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompMovement         *movement   = &world->movements[id];
            CompBody             *body       = &world->bodies[id];
            CompController       *controller = &world->controllers[id];
            const MoveStateForce *force      = &MOVE_STATE_FORCES[movement->configId][movement->moveState];

            vec3s vel     = body->vel;
            vec2s wishdir = controller->wishdir2;

            switch (movement->moveState)
            {
            case MOVE_IDLE:
                if (glms_vec2_norm(wishdir) > 0)
                    movement->moveState = MOVE_WALK;
                break;
            }

            vel = glms_vec3_add(body->vel, glms_vec3_scale((vec3s){wishdir.x, wishdir.y, 0}, force->accell * fdt));

            body->vel = vel;
        }
    }
}
