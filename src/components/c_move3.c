#include "component.h"

const char *move_state_name[MOVE_STATE_COUNT] = {
    [MOVE_IDLE] = "Idle",         [MOVE_WALK] = "Walk",     [MOVE_STUN] = "Stun",       [MOVE_FALL] = "Fall",
    [MOVE_JUMP] = "Jump",         [MOVE_CROUCH] = "Crouch", [MOVE_SLIDE] = "Slide",     [MOVE_WALLRUN] = "Wallrun",
    [MOVE_WALLJUMP] = "Walljump", [MOVE_MANTLE] = "Mantle", [MOVE_LANDING] = "Landing", [MOVE_FLY] = "Fly",
    [MOVE_DEAD] = "Dead",
};