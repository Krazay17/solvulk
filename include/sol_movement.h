#pragma once
#include "world.h"
#include <cglm/types-struct.h>

typedef enum
{
    MOVE_CONFIG_PLAYER,
    MOVE_CONFIG_WIZARD,
    MOVE_CONFIG_COUNT,
} MoveConfigId;

typedef enum
{
    MOVE_IDLE,
    MOVE_WALK,
    MOVE_FALL,
    MOVE_JUMP,
    MOVE_SLIDE,
    MOVE_FLY,
    MOVE_STATE_COUNT
} MoveState;

typedef struct
{
    float speed, accell, friction, gravity;
} MoveStateForce;

typedef void (*StateUpdate)(World *world, int id, float dt);
typedef void (*StateEnter)(World *world, int id);
typedef void (*StateExit)(World *world, int id);
typedef bool (*StateCanExit)(World *world, int id);
typedef bool (*StateCanEnter)(World *world, int id);

typedef struct
{
    StateUpdate update;
    StateEnter enter;
    StateExit exit;
    StateCanExit canExit;
    StateCanEnter canEnter;
} MoveStateFunc;

extern const MoveStateForce MOVE_STATE_FORCES[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT];
extern const MoveStateFunc MOVE_STATE_FUNCS[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT];

vec3s ApplyFriction3(vec3s wishdir, vec3s prevvel, float friction, float dt);
vec3s ApplyAccel3(vec3s wishdir, vec3s prevvel, float speed, float accel, float dt);

void Sol_Movement_SetState(World *world, int id, MoveState state);

void Sol_Movement_Idle_Update(World *world, int id, float dt);
void Sol_Movement_Idle_Enter(World *world, int id);
void Sol_Movement_Idle_Exit(World *world, int id);

void Sol_Movement_Walk_Update(World *world, int id, float dt);
void Sol_Movement_Walk_Enter(World *world, int id);
void Sol_Movement_Walk_Exit(World *world, int id);