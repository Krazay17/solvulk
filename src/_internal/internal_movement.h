#pragma once

#include <stdbool.h>
#include <cglm/types-struct.h>

typedef struct World World;

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

bool Sol_Movement_SetState(World *world, int id, MoveState state);

void Sol_Movement_Idle_Update(World *world, int id, float dt);
void Sol_Movement_Idle_Enter(World *world, int id);
void Sol_Movement_Idle_Exit(World *world, int id);
bool Sol_Movement_Idle_CanEnter(World *world, int id);
bool Sol_Movement_Idle_CanExit(World *world, int id);

void Sol_Movement_Walk_Update(World *world, int id, float dt);
void Sol_Movement_Walk_Enter(World *world, int id);
void Sol_Movement_Walk_Exit(World *world, int id);
bool Sol_Movement_Walk_CanEnter(World *world, int id);
bool Sol_Movement_Walk_CanExit(World *world, int id);

void Sol_Movement_Jump_Update(World *world, int id, float dt);
void Sol_Movement_Jump_Enter(World *world, int id);
void Sol_Movement_Jump_Exit(World *world, int id);
bool Sol_Movement_Jump_CanEnter(World *world, int id);
bool Sol_Movement_Jump_CanExit(World *world, int id);

void Sol_Movement_Fall_Update(World *world, int id, float dt);
void Sol_Movement_Fall_Enter(World *world, int id);
void Sol_Movement_Fall_Exit(World *world, int id);
bool Sol_Movement_Fall_CanEnter(World *world, int id);
bool Sol_Movement_Fall_CanExit(World *world, int id);

void Sol_Movement_Dash_Update(World *world, int id, float dt);
void Sol_Movement_Dash_Enter(World *world, int id);
void Sol_Movement_Dash_Exit(World *world, int id);
bool Sol_Movement_Dash_CanEnter(World *world, int id);
bool Sol_Movement_Dash_CanExit(World *world, int id);

void Sol_Movement_Fly_Update(World *world, int id, float dt);
void Sol_Movement_Fly_Enter(World *world, int id);
void Sol_Movement_Fly_Exit(World *world, int id);
bool Sol_Movement_Fly_CanEnter(World *world, int id);
bool Sol_Movement_Fly_CanExit(World *world, int id);

