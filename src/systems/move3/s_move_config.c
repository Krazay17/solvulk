#include "move3/s_move3.h"

const char *move_state_name[MOVE_STATE_COUNT] = {
    [MOVE_IDLE]     = "Idle",     //
    [MOVE_RUN]      = "Run",      //
    [MOVE_WALK]     = "Walk",     //
    [MOVE_STUN]     = "Stun",     //
    [MOVE_FALL]     = "Fall",     //
    [MOVE_JUMP]     = "Jump",     //
    [MOVE_CROUCH]   = "Crouch",   //
    [MOVE_SLIDE]    = "Slide",    //
    [MOVE_WALLRUN]  = "Wallrun",  //
    [MOVE_WALLJUMP] = "Walljump", //
    [MOVE_MANTLE]   = "Mantle",   //
    [MOVE_LANDING]  = "Landing",  //
    [MOVE_FLY]      = "Fly",      //
    [MOVE_DEAD]     = "Dead",     //
    [MOVE_DASH]     = "Dash",     //
};

const MoveState MOVE_STATE_PRIORITY[MOVE_STATE_COUNT] = {
    MOVE_DEAD,     //
    MOVE_STUN,     //
    MOVE_DASH,     //
    MOVE_LANDING,  //
    MOVE_MANTLE,   //
    MOVE_WALLJUMP, //
    MOVE_WALLRUN,  //
    MOVE_JUMP,     //
    MOVE_SLIDE,    //
    MOVE_FALL,     //
    MOVE_FLY,      //
    MOVE_CROUCH,   //
    MOVE_WALK,     //
    MOVE_RUN,      //
    MOVE_IDLE,     //
};

const MoveStateForce MOVE_STATE_FORCES[MOVEMENTKIND_COUNT][MOVE_STATE_COUNT] =
    {
        [MOVEMENTKIND_DUDE] =
            {
                [MOVE_IDLE]     = {.speed = 0.0f, .accell = 0.0f, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_WALK]     = {.speed = 3.0f, .accell = 8.0f, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_RUN]      = {.speed = 7.0f, .accell = 12.0f, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_DASH]     = {.speed = 7.0f, .accell = 4.0f, .friction = 0.0f, .gravity = -13.0f},
                [MOVE_CROUCH]   = {.speed = 4.0f, .accell = 20.0f, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_FALL]     = {.speed = 5.0f, .accell = 3.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_JUMP]     = {.speed = 5.0f, .accell = 4.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_WALLJUMP] = {.speed = 5.0f, .accell = 2.0f, .friction = 0.5f, .gravity = -13.0f},
                [MOVE_WALLRUN]  = {.speed = 12.0f, .accell = 1.0f, .friction = 1.0f, .gravity = -2.0f},
                [MOVE_MANTLE]   = {.speed = 6.0f, .accell = 1.0f, .friction = 0.0f, .gravity = 0.0f},
                [MOVE_SLIDE]    = {.speed = 2.0f, .accell = 1.0f, .friction = 0.66f, .gravity = -13.0f},
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
                [MOVE_IDLE]    = {.speed = 0.0f, .accell = 0.0f, .friction = 25.0f, .gravity = -13.0f},
                [MOVE_WALK]    = {.speed = 5.0f, .accell = 20.0f, .friction = 8.0f, .gravity = -13.0f},
                [MOVE_RUN]     = {.speed = 5.0f, .accell = 20.0f, .friction = 8.0f, .gravity = -13.0f},
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