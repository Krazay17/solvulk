#include "move3/s_move3.h"

const MoveState MOVE_STATE_PRIORITY[MOVE_STATE_COUNT] = {
    MOVE_DEAD,     //
    MOVE_STUN,     //
    MOVE_LANDING,  //
    MOVE_JUMP,     //
    MOVE_MANTLE,   //
    MOVE_WALLJUMP, //
    MOVE_WALLRUN,  //
    MOVE_SLIDE,    //
    MOVE_FALL,     //
    MOVE_FLY,      //
    MOVE_CROUCH,   //
    MOVE_WALK,     //
    MOVE_IDLE,     //
};

const MoveStateForce MOVE_STATE_FORCES[MOVEMENTKIND_COUNT][MOVE_STATE_COUNT] =
{
        [MOVEMENTKIND_PLAYER] =
            {
                [MOVE_IDLE]     = {.speed = 0.0f, .accell = 0.0f, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_WALK]     = {.speed = 7.0f, .accell = 12.0f, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_CROUCH]   = {.speed = 4.0f, .accell = 20.0f, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_FALL]     = {.speed = 5.0f, .accell = 3.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_JUMP]     = {.speed = 5.0f, .accell = 4.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_WALLJUMP] = {.speed = 5.0f, .accell = 2.0f, .friction = 0.5f, .gravity = -13.0f},
                [MOVE_WALLRUN]  = {.speed = 12.0f, .accell = 1.0f, .friction = 1.0f, .gravity = -2.0f},
                [MOVE_MANTLE]   = {.speed = 6.0f, .accell = 1.0f, .friction = 0.0f, .gravity = 0.0f},
                [MOVE_SLIDE]    = {.speed = 2.5f, .accell = 1.0f, .friction = 0.66f, .gravity = -13.0f},
                [MOVE_DEAD]     = {.speed = 0.0f, .accell = 0.0f, .friction = 1.0f, .gravity = -13.0f},
                [MOVE_FLY]      = {.speed = 4.0f, .accell = 7.0f, .friction = 1.0f, .gravity = 0.0f},
                [MOVE_STUN]     = {.speed = 0.0f, .accell = 0.0f, .friction = 5.1f, .gravity = -13.0f},
                [MOVE_LANDING]  = {.duration = 0.6f},
            },
        [MOVEMENTKIND_SPECTATE] =
            {
                [MOVE_FLY] = {.speed = 10.0f, .accell = 7.0f, .friction = 1.0f, .gravity = 0},
            },
        [MOVEMENTKIND_WIZARD] =
            {
                [MOVE_IDLE]    = {.speed = 0, .accell = 0, .friction = 25.0f, .gravity = -13.0f},
                [MOVE_WALK]    = {.speed = 5.0f, .accell = 20.0f, .friction = 8.0f, .gravity = -13.0f},
                [MOVE_CROUCH]  = {.speed = 4.0f, .accell = 15.0f, .friction = 8.0f, .gravity = -13.0f},
                [MOVE_FALL]    = {.speed = 5.0f, .accell = 5.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_WALLRUN] = {.speed = 8.0f, .accell = 1.0f, .friction = 0.1f, .gravity = -2.0f},
                [MOVE_JUMP]    = {.speed = 5.0f, .accell = 10.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_SLIDE]   = {.speed = 3.0f, .accell = 0.1f, .friction = 2.0f, .gravity = -13.0f},
                [MOVE_DEAD]    = {.speed = 0, .accell = 0, .friction = 1.0f, .gravity = -13.0f},
                [MOVE_FLY]     = {.speed = 5.0f, .accell = 3.5f, .friction = 1.0f, .gravity = 0},
                [MOVE_STUN]    = {.speed = 0.0f, .accell = 0.0f, .friction = 5.1f, .gravity = -13.0f},
            },
};

const MoveStateFunc MOVE_STATE_FUNCS[MOVE_STATE_COUNT] = {
    [MOVE_IDLE] =
        {
            Move_Idle_Update,
            Move_Idle_Enter,
            Move_Idle_Exit,
            Move_Idle_CanExit,
            Move_Idle_CanEnter,
        },
    [MOVE_WALK] =
        {
            Move_Walk_Update,
            Move_Walk_Enter,
            Move_Walk_Exit,
            Move_Walk_CanExit,
            Move_Walk_CanEnter,
        },
    [MOVE_FALL] =
        {
            Move_Fall_Update,
            Move_Fall_Enter,
            Move_Fall_Exit,
            Move_Fall_CanExit,
            Move_Fall_CanEnter,
        },
    [MOVE_JUMP] =
        {
            Move_Jump_Update,
            Move_Jump_Enter,
            Move_Jump_Exit,
            Move_Jump_CanExit,
            Move_Jump_CanEnter,
        },
    [MOVE_FLY] =
        {
            Move_Fly_Update,
            Move_Fly_Enter,
            Move_Fly_Exit,
            Move_Fly_CanExit,
            Move_Fly_CanEnter,
        },
    [MOVE_CROUCH] =
        {
            Move_Crouch_Update,
            Move_Crouch_Enter,
            Move_Crouch_Exit,
            Move_Crouch_CanExit,
            Move_Crouch_CanEnter,
        },
    [MOVE_SLIDE] =
        {
            Move_Slide_Update,
            Move_Slide_Enter,
            Move_Slide_Exit,
            Move_Slide_CanExit,
            Move_Slide_CanEnter,
        },
    [MOVE_WALLRUN] =
        {
            Move_Wallrun_Update,
            Move_Wallrun_Enter,
            Move_Wallrun_Exit,
            Move_Wallrun_CanExit,
            Move_Wallrun_CanEnter,
        },
    [MOVE_WALLJUMP] =
        {
            Move_Walljump_Update,
            Move_Walljump_Enter,
            Move_Walljump_Exit,
            Move_Walljump_CanExit,
            Move_Walljump_CanEnter,
        },
    [MOVE_DEAD] =
        {
            Move_Dead_Update,
            Move_Dead_Enter,
            Move_Dead_Exit,
            Move_Dead_CanExit,
            Move_Dead_CanEnter,
        },
    [MOVE_STUN] =
        {
            Move_Stun_Update,
            Move_Stun_Enter,
            Move_Stun_Exit,
            Move_Stun_CanExit,
            Move_Stun_CanEnter,
        },
    [MOVE_MANTLE] =
        {
            Move_Mantle_Update,
            Move_Mantle_Enter,
            Move_Mantle_Exit,
            Move_Mantle_CanExit,
            Move_Mantle_CanEnter,
            Move_Mantle_Draw,
        },
    [MOVE_LANDING] =
        {
            Move_Landing_Update,
            Move_Landing_Enter,
            Move_Landing_Exit,
            Move_Landing_CanExit,
            Move_Landing_CanEnter,
            Move_Landing_Draw,
        },
};
