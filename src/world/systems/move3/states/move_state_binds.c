#include "move3/s_move3.h"

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
    // [MOVE_FLY] =
    //     {
    //         Sol_Movement_Fly_Update,
    //         Sol_Movement_Fly_Enter,
    //         Sol_Movement_Fly_Exit,
    //         Sol_Movement_Fly_CanExit,
    //         Sol_Movement_Fly_CanEnter,
    //     },
    // [MOVE_CROUCH] =
    //     {
    //         Crouch_State_Update,
    //         Crouch_State_Enter,
    //         Crouch_State_Exit,
    //         Crouch_State_CanExit,
    //         Crouch_State_CanEnter,
    //     },
    // [MOVE_SLIDE] =
    //     {
    //         Slide_State_Update,
    //         Slide_State_Enter,
    //         Slide_State_Exit,
    //         Slide_State_CanExit,
    //         Slide_State_CanEnter,
    //     },
    [MOVE_WALLRUN] =
        {
            Wallrun_State_Update,
            Wallrun_State_Enter,
            Wallrun_State_Exit,
            Wallrun_State_CanExit,
            Wallrun_State_CanEnter,
        },
    // [MOVE_WALLJUMP] =
    //     {
    //         Walljump_State_Update,
    //         Walljump_State_Enter,
    //         Walljump_State_Exit,
    //         Walljump_State_CanExit,
    //         Walljump_State_CanEnter,
    //     },
    // [MOVE_DEAD] =
    //     {
    //         Dead_State_Update,
    //         Dead_State_Enter,
    //         Dead_State_Exit,
    //         Dead_State_CanExit,
    //         Dead_State_CanEnter,
    //     },
    // [MOVE_STUN] =
    //     {
    //         Stun_State_Update,
    //         Stun_State_Enter,
    //         Stun_State_Exit,
    //         Stun_State_CanExit,
    //         Stun_State_CanEnter,
    //     },
    [MOVE_MANTLE] =
        {
            Mantle_State_Update,
            Mantle_State_Enter,
            Mantle_State_Exit,
            Mantle_State_CanExit,
            Mantle_State_CanEnter,
            Mantle_State_Draw,
        },
};
