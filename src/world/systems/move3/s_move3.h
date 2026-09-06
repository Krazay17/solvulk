#pragma once
#include "sol_math.h"

#include "estate.h"

#define WALKABLE_SLOPE 0.7f
#define MAX_WALK_DISTANCE_BUFFER 0.2f

typedef struct ScMove3 ScMove3;

typedef struct
{
    float speed, accell, friction, gravity, duration;
} MoveStateForce;

extern const MoveState      MOVE_STATE_PRIORITY[MOVE_STATE_COUNT];
extern const MoveStateFunc  MOVE_STATE_FUNCS[MOVE_STATE_COUNT];
extern const MoveStateForce MOVE_STATE_FORCES[MOVEMENTKIND_COUNT][MOVE_STATE_COUNT];

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

void Move3_EvaluateState(World *world, int id, ScMove3 *move, ScCmd *cmd);

void Move3_CommitState(World *world, int id, MoveState target_state,const MoveStateFunc *current_state_func,
                       const MoveStateFunc *target_state_func, ScMove3 *move, ScCmd *cmd);
void Knockback(World *world, int id, ScMove3 *move, float fdt);
void CrouchHeight(World *world, int id, ScMove3 *move, float fdt);
void RestoreFriction(World *world, int id, ScMove3 *move, float fdt);
void GroundCheck(World *world, int id, ScMove3 *move, float fdt);

typedef void (*MoveStateUpdate)(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
typedef void (*MoveStateEnter)(World *world, int id, ScMove3 *move, ScCmd *cmd);
typedef void (*MoveStateExit)(World *world, int id, ScMove3 *move, ScCmd *cmd);
typedef bool (*MoveStateCanExit)(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
typedef bool (*MoveStateCanEnter)(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);
typedef void (*MoveStateDraw)(World *world, int id, ScMove3 *move, ScCmd *cmd);

void Move_Idle_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Idle_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Idle_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Idle_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Idle_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Walk_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Walk_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Walk_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Walk_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Walk_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Jump_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Jump_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Jump_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Jump_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Jump_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Fall_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Fall_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Fall_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Fall_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Fall_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Fly_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Fly_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Fly_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Fly_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Fly_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Crouch_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Crouch_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Crouch_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Crouch_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Crouch_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Slide_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Slide_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Slide_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Slide_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Slide_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Wallrun_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Wallrun_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Wallrun_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Wallrun_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Wallrun_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Walljump_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Walljump_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Walljump_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Walljump_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Walljump_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Dead_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Dead_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Dead_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Dead_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Dead_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Stun_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Stun_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Stun_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Stun_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Stun_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);

void Move_Mantle_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Mantle_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Mantle_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Mantle_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Mantle_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);
void Move_Mantle_Draw(World *world, int id, ScMove3 *move, ScCmd *cmd, double dt);

void Move_Landing_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
void Move_Landing_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd);
void Move_Landing_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd);
bool Move_Landing_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
bool Move_Landing_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);
void Move_Landing_Draw(World *world, int id, ScMove3 *move, ScCmd *cmd, double dt);
