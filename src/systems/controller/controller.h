#pragma once

#include "sol/types.h"

static const PlayerActionStates action_binds[SOL_KEY_COUNT] = {
    [SOL_KEY_W] = ACTION_FWD,
    [SOL_KEY_A] = ACTION_LEFT,
    [SOL_KEY_S] = ACTION_BWD,
    [SOL_KEY_D] = ACTION_RIGHT,
    [SOL_KEY_F] = 0,
    [SOL_KEY_SPACE] = ACTION_JUMP,
    [SOL_KEY_ESCAPE] = 0,
    [SOL_KEY_SHIFT] = ACTION_DASH,
};