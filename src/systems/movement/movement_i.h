#pragma once
#include "sol/sol.h"

#include "estate.h"
#include "movement.h"

typedef struct
{
    float speed, accell, friction, gravity;
} MoveStateForce;

extern const MoveStateForce MOVE_STATE_FORCES[MOVEMENTKIND_COUNT][MOVE_STATE_COUNT];
extern const StateFunc      MOVE_STATE_FUNCS[MOVE_STATE_COUNT];

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

void Sol_Movement_Prestep(World *world, double dt, double time);

void  CrouchHeight(World *world, int id, float fdt);
vec3s GroundSlope(World *world, int id);
vec3s ProjectOntoGround(World *world, int id, vec3s wishdir);
void  Knockback(World *world, int id, float fdt);
void RestoreFriction(World *world, int id, CompMovement *move, float fdt);

void Sol_Movement_Idle_Update(World *world, int id, float dt);
void Sol_Movement_Idle_Enter(World *world, int id);
void Sol_Movement_Idle_Exit(World *world, int id);
bool Sol_Movement_Idle_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Idle_CanEnter(World *world, int id, u32 last, u32 next);

void Sol_Movement_Walk_Update(World *world, int id, float dt);
void Sol_Movement_Walk_Enter(World *world, int id);
void Sol_Movement_Walk_Exit(World *world, int id);
bool Sol_Movement_Walk_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Walk_CanEnter(World *world, int id, u32 last, u32 next);

void Sol_Movement_Jump_Update(World *world, int id, float dt);
void Sol_Movement_Jump_Enter(World *world, int id);
void Sol_Movement_Jump_Exit(World *world, int id);
bool Sol_Movement_Jump_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Jump_CanEnter(World *world, int id, u32 last, u32 next);

void Sol_Movement_Fall_Update(World *world, int id, float dt);
void Sol_Movement_Fall_Enter(World *world, int id);
void Sol_Movement_Fall_Exit(World *world, int id);
bool Sol_Movement_Fall_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Fall_CanEnter(World *world, int id, u32 last, u32 next);

void Sol_Movement_Fly_Update(World *world, int id, float dt);
void Sol_Movement_Fly_Enter(World *world, int id);
void Sol_Movement_Fly_Exit(World *world, int id);
bool Sol_Movement_Fly_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Fly_CanEnter(World *world, int id, u32 last, u32 next);

void Crouch_State_Update(World *world, int id, float dt);
void Crouch_State_Enter(World *world, int id);
void Crouch_State_Exit(World *world, int id);
bool Crouch_State_CanExit(World *world, int id, u32 next);
bool Crouch_State_CanEnter(World *world, int id, u32 last, u32 next);

void Slide_State_Update(World *world, int id, float dt);
void Slide_State_Enter(World *world, int id);
void Slide_State_Exit(World *world, int id);
bool Slide_State_CanExit(World *world, int id, u32 next);
bool Slide_State_CanEnter(World *world, int id, u32 last, u32 next);

void Wallrun_State_Update(World *world, int id, float dt);
void Wallrun_State_Enter(World *world, int id);
void Wallrun_State_Exit(World *world, int id);
bool Wallrun_State_CanExit(World *world, int id, u32 next);
bool Wallrun_State_CanEnter(World *world, int id, u32 last, u32 next);

void Walljump_State_Update(World *world, int id, float dt);
void Walljump_State_Enter(World *world, int id);
void Walljump_State_Exit(World *world, int id);
bool Walljump_State_CanExit(World *world, int id, u32 next);
bool Walljump_State_CanEnter(World *world, int id, u32 last, u32 next);

void Dead_State_Update(World *world, int id, float dt);
void Dead_State_Enter(World *world, int id);
void Dead_State_Exit(World *world, int id);
bool Dead_State_CanExit(World *world, int id, u32 next);
bool Dead_State_CanEnter(World *world, int id, u32 last, u32 next);
