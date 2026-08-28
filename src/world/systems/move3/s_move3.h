#pragma once
#include "sol_math.h"

#include "estate.h"

#define WALKABLE_SLOPE 0.7f

typedef struct SolMove3 SolMove3;

typedef struct
{
    float speed, accell, friction, gravity;
} MoveStateForce;

extern const StateFunc MOVE_STATE_FUNCS[];

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
    float       lerpalpha = (1.0f - dotdir);

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

static inline vec3s ProjectOntoGround(vec3s ground, vec3s wishdir)
{
    float dot = glms_vec3_dot(wishdir, ground);
    // dot = fmaxf(-0.5f, fminf(0.5, dot));
    return glms_vec3_sub(wishdir, glms_vec3_scale(ground, dot));
}

static inline vec3s GroundSlope(vec3s normal)
{
    float dot        = glms_vec3_dot(WORLD_DOWN, normal);
    vec3s projection = glms_vec3_scale(normal, dot);
    vec3s slope      = glms_vec3_sub(WORLD_DOWN, projection);
    return glms_normalize(slope);
}

static inline WallTouch CalcTouch(vec3s wallnorm, float yaw)
{
    // 1. Vector pointing FROM player TO wall
    // Since WORLD_FORWARD is {0,0,1}, facing yaw=0 means facing +Z
    float dx = -wallnorm.x;
    float dz = -wallnorm.z;

    // 2. Facing vector derived from yaw (matching atan2f(x, z) convention)
    float fwdX = sinf(yaw);
    float fwdZ = cosf(yaw);

    // 3. Relative angle between wall direction and facing direction
    float angle = atan2f(dx, dz) - atan2f(fwdX, fwdZ);

    // 4. Normalize angle to [0, 2PI]
    if (angle < 0.0f)
        angle += 2.0f * GLM_PIf;

    // 5. Offset by half a 90-degree wedge (45 degrees / PI_4)
    // This centers FRONT right in the middle of sector 0 [-45°, +45°]
    angle += GLM_PI_4f;

    // 6. Handle wrapping after offset
    if (angle >= 2.0f * GLM_PIf)
        angle -= 2.0f * GLM_PIf;

    // 7. Divide into 4 quadrants (PI_2 wide) and mask
    int sector = (int)floorf(angle / GLM_PI_2f);

    return (WallTouch)(sector & 3);
}

void  Knockback(World *world, int id, SolMove3 *move, float fdt);
void  CrouchHeight(World *world, int id, SolMove3 *move, float fdt);
void  RestoreFriction(World *world, int id, SolMove3 *move, float fdt);

void Sol_Movement_Idle_Update(World *world, int id, float dt);
void Sol_Movement_Idle_Enter(World *world, int id);
void Sol_Movement_Idle_Exit(World *world, int id);
bool Sol_Movement_Idle_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Idle_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Sol_Movement_Walk_Update(World *world, int id, float dt);
void Sol_Movement_Walk_Enter(World *world, int id);
void Sol_Movement_Walk_Exit(World *world, int id);
bool Sol_Movement_Walk_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Walk_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Sol_Movement_Jump_Update(World *world, int id, float dt);
void Sol_Movement_Jump_Enter(World *world, int id);
void Sol_Movement_Jump_Exit(World *world, int id);
bool Sol_Movement_Jump_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Jump_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Sol_Movement_Fall_Update(World *world, int id, float dt);
void Sol_Movement_Fall_Enter(World *world, int id);
void Sol_Movement_Fall_Exit(World *world, int id);
bool Sol_Movement_Fall_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Fall_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Sol_Movement_Fly_Update(World *world, int id, float dt);
void Sol_Movement_Fly_Enter(World *world, int id);
void Sol_Movement_Fly_Exit(World *world, int id);
bool Sol_Movement_Fly_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Fly_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Crouch_State_Update(World *world, int id, float dt);
void Crouch_State_Enter(World *world, int id);
void Crouch_State_Exit(World *world, int id);
bool Crouch_State_CanExit(World *world, int id, u32 next);
bool Crouch_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Slide_State_Update(World *world, int id, float dt);
void Slide_State_Enter(World *world, int id);
void Slide_State_Exit(World *world, int id);
bool Slide_State_CanExit(World *world, int id, u32 next);
bool Slide_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Wallrun_State_Update(World *world, int id, float dt);
void Wallrun_State_Enter(World *world, int id);
void Wallrun_State_Exit(World *world, int id);
bool Wallrun_State_CanExit(World *world, int id, u32 next);
bool Wallrun_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Walljump_State_Update(World *world, int id, float dt);
void Walljump_State_Enter(World *world, int id);
void Walljump_State_Exit(World *world, int id);
bool Walljump_State_CanExit(World *world, int id, u32 next);
bool Walljump_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Dead_State_Update(World *world, int id, float dt);
void Dead_State_Enter(World *world, int id);
void Dead_State_Exit(World *world, int id);
bool Dead_State_CanExit(World *world, int id, u32 next);
bool Dead_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Stun_State_Update(World *world, int id, float dt);
void Stun_State_Enter(World *world, int id);
void Stun_State_Exit(World *world, int id);
bool Stun_State_CanExit(World *world, int id, u32 next);
bool Stun_State_CanEnter(World *world, int id, u32 last, u32 next, int slot);

void Mantle_State_Update(World *world, int id, float dt);
void Mantle_State_Enter(World *world, int id);
void Mantle_State_Exit(World *world, int id);
bool Mantle_State_CanExit(World *world, int id, u32 nextState);
bool Mantle_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot);
void Mantle_State_Draw(World *world, int id, double dt);
