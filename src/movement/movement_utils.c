#include "movement.h"
#include "sol_core.h"

vec3s ApplyFriction3(vec3s wishdir, vec3s prevvel, float friction, float dt)
{
    vec3s       vel   = prevvel;
    const float speed = glms_vec3_norm(vel);
    if (speed < 0.01f)
        return (vec3s){0.0f, 0.0f, 0.0f};
    const float drop     = speed * friction * dt;
    const float newspeed = fmaxf(0.0f, speed - drop);
    vel                  = glms_vec3_scale(vel, newspeed / speed);

    return vel;
}

vec3s ApplyAccel3(vec3s wishdir, vec3s prevvel, float speed, float accel, float dt)
{
    vec3s vel = prevvel;

    if (glms_vec3_norm(wishdir) == 0)
        return vel;
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

StrafeDir Get_StrafeDir(float x, float z, float xB, float zB)
{
    float angle = atan2f(x, z) - atan2f(xB, zB);
    if (angle < 0)
        angle += 2.0f * M_PI;

    return (int)((angle / (M_PI / 2.0f)) + 0.5f) % 4;
}
