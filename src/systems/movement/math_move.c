#include "sol_core.h"

#include "movement_i.h"

vec3s ApplyFriction3(vec3s wishdir, vec3s prevvel, float friction, float dt)
{
    const float speed = glms_vec3_norm(prevvel);
    if (speed < 0.01f)
        return GLMS_VEC3_ZERO;
    vec3s       vel      = prevvel;
    const float drop     = speed * friction * dt;
    const float newspeed = fmaxf(0.0f, speed - drop);
    vel                  = glms_vec3_scale(vel, newspeed / speed);

    return vel;
}

vec3s ApplyAccel3(vec3s wishdir, vec3s prevvel, float speed, float accel, float dt)
{
    if (glms_vec3_norm(wishdir) == 0)
        return prevvel;

    vec3s       vel       = prevvel;
    const float dotdir    = glms_vec3_dot(wishdir, glms_vec3_normalize(vel));
    float       lerpalpha = (1.0f - dotdir) * 0.5f;

    vec3s projected     = glms_vec3_proj(vel, wishdir);
    float steerStrength = fminf(1.0f, accel * dt);
    vel                 = glms_vec3_lerp(vel, projected, lerpalpha * steerStrength);

    const float dirspeed = glms_vec3_dot(vel, wishdir);
    const float addspeed = speed - dirspeed;
    if (addspeed > 0)
    {
        const float accelspeed = accel * speed * dt;
        const float finaladd   = fminf(accelspeed, addspeed);
        vel                    = glms_vec3_add(vel, glms_vec3_scale(wishdir, finaladd));
    }
    return vel;
}
