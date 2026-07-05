#pragma once
#include "base.h"

typedef struct World World;

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
    MOVE_MANTLE,
    MOVE_FLY,
    MOVE_DEAD,
    MOVE_STATE_COUNT
} MoveState;
typedef struct
{
    u8     kind;
    double lastEntered, lastExited;
    float  elapsed, accum;
    union {
        struct
        {
            vec3s pos;
            float dist;
        } mantle;
    } as;
    vec3s enterVel, dir;
} MoveStateData;
typedef enum
{
    MOVEMENTKIND_PLAYER,
    MOVEMENTKIND_WIZARD,
    MOVEMENTKIND_COUNT,
} MovementKind;
typedef struct CompMovement
{
    u8        kind;
    MoveState state;
    vec3s     updir, dashdir, wallNormal, lastTouch, knockVel, lastMoveDir;
    float     baseHeight, targetHeight;
    float     speedMod, frictionMod, knockDur;
    float     wallDot;
    float     airtime, groundtime;

    bool          wantsJump, jumpPressedLastFrame;
    MoveStateData stateData[MOVE_STATE_COUNT];
} CompMovement;

void  Sol_Movement_Init(World *world);
void  Sol_Movement_Add(World *world, int id, MovementKind kind);
void  Sol_Movement_SetSpeedMod(World *world, int id, float amnt);
bool  Sol_Movement_SetState(World *world, int id, MoveState state);
void  Sol_Movement_ForceState(World *world, int id, MoveState nextState);
void  Sol_Movement_SetKnockback(World *world, int id, vec3s vel, float duration);
u32   Sol_Movement_GetState(World *world, int id);
float Sol_Movement_GetGroundtime(World *world, int id);
float Sol_Movement_GetAirtime(World *world, int id);