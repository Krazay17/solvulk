#pragma once
#include "sol/types.h"

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

static const MoveStateForce MOVE_STATE_FORCES[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT] = {
    [MOVE_CONFIG_PLAYER] =
        {
            [MOVE_IDLE]  = {.speed = 0, .accell = 0, .friction = 10.0f, .gravity = 0},
            [MOVE_WALK]  = {.speed = 6.0f, .accell = 50.0f, .friction = 5.0f, .gravity = 9.81f},
            [MOVE_FALL]  = {.speed = 6.0f, .accell = 5.0f, .friction = 0.0f, .gravity = 9.81f},
            [MOVE_JUMP]  = {.speed = 6.0f, .accell = 5.0f, .friction = 0.0f, .gravity = 9.81f},
            [MOVE_DASH]  = {.speed = 24.0f, .accell = 32.0f, .friction = 0.0f, .gravity = 0},
            [MOVE_SLIDE] = {.speed = 6.0f, .accell = 5.0f, .friction = 1.0f, .gravity = 9.81f},
            [MOVE_FLY]   = {.speed = 6.0f, .accell = 10.0f, .friction = 1.0f, .gravity = 0},
        },
    [MOVE_CONFIG_WIZARD] =
        {
            [MOVE_IDLE]  = {.speed = 0, .accell = 0, .friction = 15.0f, .gravity = 0},
            [MOVE_WALK]  = {.speed = 5.0f, .accell = 20.0f, .friction = 5.0f, .gravity = 9.81f},
            [MOVE_FALL]  = {.speed = 5.0f, .accell = 5.0f, .friction = 0.0f, .gravity = 9.81f},
            [MOVE_JUMP]  = {.speed = 5.0f, .accell = 5.0f, .friction = 0.0f, .gravity = 9.81f},
            [MOVE_SLIDE] = {.speed = 5.0f, .accell = 5.0f, .friction = 1.0f, .gravity = 9.81f},
            [MOVE_FLY]   = {.speed = 5.0f, .accell = 5.0f, .friction = 1.0f, .gravity = 0},
        },
};

static const MoveStateFunc MOVE_STATE_FUNCS[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT] = {
    [MOVE_CONFIG_PLAYER] =
        {
            [MOVE_IDLE] =
                {
                    .enter    = Sol_Movement_Idle_Enter,
                    .exit     = Sol_Movement_Idle_Exit,
                    .update   = Sol_Movement_Idle_Update,
                    .canExit  = Sol_Movement_Idle_CanExit,
                    .canEnter = Sol_Movement_Idle_CanEnter,
                },
            [MOVE_WALK] =
                {
                    .enter    = Sol_Movement_Walk_Enter,
                    .exit     = Sol_Movement_Walk_Exit,
                    .update   = Sol_Movement_Walk_Update,
                    .canExit  = Sol_Movement_Walk_CanExit,
                    .canEnter = Sol_Movement_Walk_CanEnter,
                },
            [MOVE_FALL] =
                {
                    .enter    = Sol_Movement_Fall_Enter,
                    .exit     = Sol_Movement_Fall_Exit,
                    .update   = Sol_Movement_Fall_Update,
                    .canExit  = Sol_Movement_Fall_CanExit,
                    .canEnter = Sol_Movement_Fall_CanEnter,
                },
            [MOVE_JUMP] =
                {
                    .enter    = Sol_Movement_Jump_Enter,
                    .exit     = Sol_Movement_Jump_Exit,
                    .update   = Sol_Movement_Jump_Update,
                    .canExit  = Sol_Movement_Jump_CanExit,
                    .canEnter = Sol_Movement_Jump_CanEnter,
                },
            [MOVE_DASH] =
                {
                    .enter    = Sol_Movement_Dash_Enter,
                    .exit     = Sol_Movement_Dash_Exit,
                    .update   = Sol_Movement_Dash_Update,
                    .canExit  = Sol_Movement_Dash_CanExit,
                    .canEnter = Sol_Movement_Dash_CanEnter,
                },
            [MOVE_FLY] =
                {
                    .enter    = Sol_Movement_Fly_Enter,
                    .exit     = Sol_Movement_Fly_Exit,
                    .update   = Sol_Movement_Fly_Update,
                    .canExit  = Sol_Movement_Fly_CanExit,
                    .canEnter = Sol_Movement_Fly_CanEnter,
                },
        },
    [MOVE_CONFIG_WIZARD] =
        {
            [MOVE_IDLE] =
                {
                    .enter    = Sol_Movement_Idle_Enter,
                    .exit     = Sol_Movement_Idle_Exit,
                    .update   = Sol_Movement_Idle_Update,
                    .canExit  = Sol_Movement_Idle_CanExit,
                    .canEnter = Sol_Movement_Idle_CanEnter,
                },
            [MOVE_WALK] =
                {
                    .enter    = Sol_Movement_Walk_Enter,
                    .exit     = Sol_Movement_Walk_Exit,
                    .update   = Sol_Movement_Walk_Update,
                    .canExit  = Sol_Movement_Walk_CanExit,
                    .canEnter = Sol_Movement_Walk_CanEnter,
                },
        },
        
};