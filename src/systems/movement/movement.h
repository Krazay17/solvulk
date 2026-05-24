#pragma once

typedef enum
{
    STRAFE_FWD,
    STRAFE_FWD_LEFT,
    STRAFE_LEFT,
    STRAFE_BWD_LEFT,
    STRAFE_BWD,
    STRAFE_BWD_RIGHT,
    STRAFE_RIGHT,
    STRAFE_FWD_RIGHT,
} StrafeDir;

typedef enum
{
    MOVE_IDLE,
    MOVE_WALK,
    MOVE_FALL,
    MOVE_JUMP,
    MOVE_CROUCH,
    MOVE_SLIDE,
    MOVE_WALLRUN,
    MOVE_WALLJUMP,
    MOVE_FLY,
    MOVE_STATE_COUNT
} MoveState;

typedef struct
{
    MoveConfigId configId;
} MovementDesc;
void Sol_Movement_Init(World *world);
void Sol_Movement_Add(World *world, int id, MovementDesc desc);
void Sol_System_Movement_2d_Step(World *world, double dt, double time);
void Sol_System_Movement_3d_Step(World *world, double dt, double time);
void Sol_Movement_SetSpeedMod(World *world, int id, float amnt);

static inline StrafeDir Sol_GetStrafedir(float x, float z, float bX, float bZ)
{
    // 1. Get the relative angle between velocity and facing direction
    float angle = atan2f(x, z) - atan2f(bX, bZ);

    // 2. Keep the angle strictly positive within [0, 2PI]
    if (angle < 0.0f)
        angle += 2.0f * GLM_PIf;

    // 3. Offset by half a wedge (22.5 degrees) so cardinal directions
    // sit right in the middle of our integer sectors instead of on the edges.
    angle += GLM_PI_4f * 0.5f;

    // 4. Handle wrapping after the offset addition
    if (angle >= 2.0f * GLM_PIf)
        angle -= 2.0f * GLM_PIf;

    // 5. Use floorf to establish clean boundaries, then cast
    int sector = (int)floorf(angle / GLM_PI_4f);

    // Safety clamp to guarantee it maps to your 0-7 enum range
    return (StrafeDir)(sector & 7);
}

static inline vec3s ApplyFriction3(vec3s wishdir, vec3s prevvel, float friction, float dt)
{
    const float speed = glms_vec3_norm(prevvel);
    if (speed < 0.1f)
        return GLMS_VEC3_ZERO;
    vec3s       vel      = prevvel;
    const float drop     = speed * friction * dt;
    const float newspeed = fmaxf(0.0f, speed - drop);
    vel                  = glms_vec3_scale(vel, newspeed / speed);

    return vel;
}

static inline vec3s ApplyAccel3(vec3s wishdir, vec3s prevvel, float speed, float accel, float dt)
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