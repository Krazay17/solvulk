#include "movement_i.h"

const MoveStateForce MOVE_STATE_FORCES[MOVEMENTKIND_COUNT][MOVE_STATE_COUNT] =
    {
        [MOVEMENTKIND_PLAYER] =
            {
                [MOVE_IDLE]     = {.speed = 0, .accell = 0, .friction = 25.0f, .gravity = -11.81f},
                [MOVE_WALK]     = {.speed = 6.5f, .accell = 25.0f, .friction = 8.0f, .gravity = -11.81f},
                [MOVE_CROUCH]   = {.speed = 4.0f, .accell = 15.0f, .friction = 8.0f, .gravity = -11.81f},
                [MOVE_FALL]     = {.speed = 4.5f, .accell = 8.0f, .friction = 0.1f, .gravity = -11.81f},
                [MOVE_JUMP]     = {.speed = 6.0f, .accell = 10.0f, .friction = 0.1f, .gravity = -11.81f},
                [MOVE_WALLJUMP] = {.speed = 4.0f, .accell = 2.0f, .friction = 0.5f, .gravity = -11.81f},
                [MOVE_WALLRUN]  = {.speed = 7.0f, .accell = 6.0f, .friction = 1.0f, .gravity = -1.0f},
                [MOVE_SLIDE]    = {.speed = 3.5f, .accell = 1.0f, .friction = 0.5f, .gravity = -11.81f},
                [MOVE_DEAD]     = {.speed = 0, .accell = 0, .friction = 1.0f, .gravity = -8.0f},
                [MOVE_FLY]      = {.speed = 4.0f, .accell = 7.0f, .friction = 1.0f, .gravity = 0},
            },
        [MOVEMENTKIND_WIZARD] =
            {
                [MOVE_IDLE]    = {.speed = 0, .accell = 0, .friction = 25.0f, .gravity = -11.81f},
                [MOVE_WALK]    = {.speed = 5.0f, .accell = 20.0f, .friction = 8.0f, .gravity = -11.81f},
                [MOVE_CROUCH]  = {.speed = 4.0f, .accell = 15.0f, .friction = 8.0f, .gravity = -11.81f},
                [MOVE_FALL]    = {.speed = 5.0f, .accell = 3.5f, .friction = 0.1f, .gravity = -11.81f},
                [MOVE_WALLRUN] = {.speed = 6.0f, .accell = 7.0f, .friction = 0.1f, .gravity = -1.0f},
                [MOVE_JUMP]    = {.speed = 5.0f, .accell = 3.5f, .friction = 0.1f, .gravity = -11.81f},
                [MOVE_SLIDE]   = {.speed = 3.0f, .accell = 0.1f, .friction = 2.0f, .gravity = -11.81f},
                [MOVE_DEAD]    = {.speed = 0, .accell = 0, .friction = 1.0f, .gravity = -9.81f},
                [MOVE_FLY]     = {.speed = 5.0f, .accell = 3.5f, .friction = 1.0f, .gravity = 0},
            },
};

const StateFunc MOVE_STATE_FUNCS[MOVE_STATE_COUNT] = {
    [MOVE_IDLE] =
        {
            Sol_Movement_Idle_Update,
            Sol_Movement_Idle_Enter,
            Sol_Movement_Idle_Exit,
            Sol_Movement_Idle_CanExit,
            Sol_Movement_Idle_CanEnter,
        },
    [MOVE_WALK] =
        {
            Sol_Movement_Walk_Update,
            Sol_Movement_Walk_Enter,
            Sol_Movement_Walk_Exit,
            Sol_Movement_Walk_CanExit,
            Sol_Movement_Walk_CanEnter,
        },
    [MOVE_FALL] =
        {
            Sol_Movement_Fall_Update,
            Sol_Movement_Fall_Enter,
            Sol_Movement_Fall_Exit,
            Sol_Movement_Fall_CanExit,
            Sol_Movement_Fall_CanEnter,
        },
    [MOVE_JUMP] =
        {
            Sol_Movement_Jump_Update,
            Sol_Movement_Jump_Enter,
            Sol_Movement_Jump_Exit,
            Sol_Movement_Jump_CanExit,
            Sol_Movement_Jump_CanEnter,
        },
    [MOVE_FLY] =
        {
            Sol_Movement_Fly_Update,
            Sol_Movement_Fly_Enter,
            Sol_Movement_Fly_Exit,
            Sol_Movement_Fly_CanExit,
            Sol_Movement_Fly_CanEnter,
        },
    [MOVE_CROUCH] =
        {
            Crouch_State_Update,
            Crouch_State_Enter,
            Crouch_State_Exit,
            Crouch_State_CanExit,
            Crouch_State_CanEnter,
        },
    [MOVE_SLIDE] =
        {
            Slide_State_Update,
            Slide_State_Enter,
            Slide_State_Exit,
            Slide_State_CanExit,
            Slide_State_CanEnter,
        },
    [MOVE_WALLRUN] =
        {
            Wallrun_State_Update,
            Wallrun_State_Enter,
            Wallrun_State_Exit,
            Wallrun_State_CanExit,
            Wallrun_State_CanEnter,
        },
    [MOVE_WALLJUMP] =
        {
            Walljump_State_Update,
            Walljump_State_Enter,
            Walljump_State_Exit,
            Walljump_State_CanExit,
            Walljump_State_CanEnter,
        },
    [MOVE_DEAD] =
        {
            Dead_State_Update,
            Dead_State_Enter,
            Dead_State_Exit,
            Dead_State_CanExit,
            Dead_State_CanEnter,
        },
};
