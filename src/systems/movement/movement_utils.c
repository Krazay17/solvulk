#include <cglm/struct.h>

#include "sol_core.h"

vec3s ApplyFriction3(vec3s wishdir, vec3s prevvel, float friction, float dt)
{
    vec3s vel = prevvel;
    float prevY = vel.y;
    vel.y = 0;

    const float speed = glms_vec3_norm(vel);
    if (speed < 0.001)
        return (vec3s){0};
    const float drop = speed * friction * dt;
    const float newspeed = fmaxf(0.0f, speed - drop);
    vel = glms_vec3_scale(vel, newspeed / speed);
    vel.y = prevY;
    return vel;
}

vec3s ApplyAccel3(vec3s wishdir, vec3s prevvel, float speed, float accel, float dt)
{
    vec3s vel = prevvel;

    if (glms_vec3_norm(wishdir) == 0)
        return vel;
    const float dotdir = glms_vec3_dot(wishdir, glms_vec3_normalize(vel));
    float lerpalpha = (1.0f - dotdir) * 0.5f;

    vec3s projected = glms_vec3_proj(vel, wishdir);
    float steerStrength = fminf(1.0f, accel * dt);
    vel = glms_vec3_lerp(vel, projected, lerpalpha * steerStrength);

    const float dirspeed = glms_vec3_dot(vel, wishdir);
    const float addspeed = speed - dirspeed;
    if (addspeed > 0)
    {
        const float accelspeed = accel * speed * dt;
        const float finaladd = fminf(accelspeed, addspeed);
        vel = glms_vec3_add(vel, glms_vec3_scale(wishdir, finaladd));
    }
    return vel;
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
    prevfunc->exit(world, id);
    movement->moveState = nextState;
    nextfunc->enter(world, id);
    Sol_Debug_Add("state", movement->moveState);
    return true;
}
