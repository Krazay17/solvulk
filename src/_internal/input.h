#pragma once
#include "sol/types.h"

typedef struct
{
    bool  mouseLocked;
    bool  keys[SOL_KEY_COUNT];
    bool  keysPrev[SOL_KEY_COUNT];
    bool  mouseButtons[SOL_MOUSE_COUNT];
    bool  mouseButtonsPrev[SOL_MOUSE_COUNT];
    float mouseWheelDelta;
    int   mouseX, mouseY;
    int   mouseDeltaX, mouseDeltaY;

    u32 action;
} LocalInput;

extern LocalInput local_input;