#include "sol_movement.h"
#include <cglm/struct.h>

static vec3s ApplyFriction3(vec3s wishdir, vec3s prevvel, float friction, float dt)
{
    vec3s latvel = prevvel;
    const float prevY = latvel.y;
    latvel.y = 0;

    const float speed = glms_vec3_norm(latvel);
    if (speed < 0.01)
        return (vec3s){0, prevY, 0};
    const float drop = speed * friction * dt;
    const float newspeed = fmaxf(0.0f, speed - drop);
    latvel = glms_vec3_scale(latvel, newspeed / speed);
    latvel.y = prevY;
    return latvel;
}

static vec3s ApplyAccel3(vec3s wishdir, vec3s prevvel, float speed, float accel, float dt)
{
    vec3s latvel = prevvel;
    const float prevY = latvel.y;
    latvel.y = 0;

    const float dirspeed = glms_vec3_norm(glms_vec3_mul(latvel, wishdir));
    const float addspeed = speed - dirspeed;
    if (addspeed <= 0)
        return prevvel;
    const float accelspeed = accel * speed * dt;
    const float final = fminf(accelspeed, addspeed);
    latvel = glms_vec3_add(latvel, glms_vec3_scale(wishdir, final));
    latvel.y = prevY;
    return latvel;
}

void Sol_Movement_SetState(World *world, int id, MoveState nextState)
{
    CompMovement *movement = &world->movements;
    if (movement->moveState == nextState)
        return;
    MoveStateFunc *prevfunc = &MOVE_STATE_FUNCS[movement->configId][movement->moveState];
    if (!prevfunc->canExit)
        return;
    MoveStateFunc *nextfunc = &MOVE_STATE_FUNCS[movement->configId][nextState];
    if (!nextfunc->canEnter)
        return;
    prevfunc->exit(world, id);
    movement->moveState = nextState;
    nextfunc->enter(world, id);
}