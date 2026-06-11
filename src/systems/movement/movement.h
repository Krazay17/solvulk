#pragma once

typedef enum
{
    MOVEMENTKIND_PLAYER,
    MOVEMENTKIND_WIZARD,
    MOVEMENTKIND_COUNT,
} MovementKind;

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
    MOVE_STUN,
    MOVE_FALL,
    MOVE_JUMP,
    MOVE_CROUCH,
    MOVE_SLIDE,
    MOVE_WALLRUN,
    MOVE_WALLJUMP,
    MOVE_FLY,
    MOVE_DEAD,
    MOVE_STATE_COUNT
} MoveState;

typedef struct
{
    double lastEntered, lastExited;
    float  elapsed, accum;
    vec3s  enterVel, dir;
} MoveStateData;

typedef struct CompMovement
{
    u8            kind;
    vec3s         updir, dashdir, wallNormal, lastTouch, knockVel;
    float         baseHeight, targetHeight;
    float         speedMod, frictionMod, knockDur;
    float         wallDot;
    MoveState     state;
    bool          wantsJump, jumpPressedLastFrame;
    MoveStateData stateData[MOVE_STATE_COUNT];
} CompMovement;

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

void Sol_Movement_Init(World *world);
void Sol_Movement_Add(World *world, int id, MovementKind kind);
void Sol_System_Movement_2d_Step(World *world, double dt, double time);
void Sol_System_Movement_3d_Step(World *world, double dt, double time);
void Sol_Movement_SetSpeedMod(World *world, int id, float amnt);
bool Sol_Movement_SetState(World *world, int id, MoveState state);
void Sol_Movement_ForceState(World *world, int id, MoveState nextState);
void Sol_Movement_SetKnockback(World *world, int id, vec3s vel, float duration);
