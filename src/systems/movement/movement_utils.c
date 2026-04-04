#include <cglm/struct.h>

#include "sol_core.h"

vec3s ApplyFriction3(vec3s wishdir, vec3s prevvel, float friction, float dt)
{
    vec3s latvel = prevvel;
    //const float prevY = latvel.y;
    //latvel.y = 0;

    const float speed = glms_vec3_norm(latvel);
    if (speed < 0.01)
        return prevvel;
    const float drop = speed * friction * dt;
    const float newspeed = fmaxf(0.0f, speed - drop);
    latvel = glms_vec3_scale(latvel, newspeed / speed);
    //latvel.y = prevY;
    return latvel;
}

vec3s ApplyAccel3(vec3s wishdir, vec3s prevvel, float speed, float accel, float dt)
{
    vec3s latvel = prevvel;
    //const float prevY = latvel.y;
    //latvel.y = 0;
    if (glms_vec3_norm(wishdir) == 0)
        return prevvel;
    const float dotdir = glms_vec3_dot(wishdir, glms_vec3_normalize(latvel));
    float lerpalpha = (1.0f - dotdir) * 0.5f;

    vec3s projected = glms_vec3_proj(latvel, wishdir);
    float steerStrength = fminf(1.0f, accel * dt);
    latvel = glms_vec3_lerp(latvel, projected, lerpalpha * steerStrength);

    const float dirspeed = glms_vec3_dot(latvel, wishdir);
    const float addspeed = speed - dirspeed;
    if (addspeed > 0)
    {
        const float accelspeed = accel * speed * dt;
        const float finaladd = fminf(accelspeed, addspeed);
        latvel = glms_vec3_add(latvel, glms_vec3_scale(wishdir, finaladd));
    }

    // const float overspeed = glms_vec3_norm(latvel);
    // if (overspeed > speed)
    //     latvel = glms_vec3_scale(latvel, speed / overspeed);
    //latvel.y = prevY;
    return latvel;
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
