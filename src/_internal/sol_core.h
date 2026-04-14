#pragma once

#include <sol/sol.h>

#include "internal_math.h"
#include "internal_world.h"
#include "internal_movement.h"
#include "internal_render.h"
#include "internal_physx.h"
#include "internal_platform.h"
#include "internal_loader.h"

typedef struct SolState
{
    volatile bool isRunning;
    volatile bool needsResize;
    float windowWidth, windowHeight;
    void *g_hwnd;
    double gameTime;

    World *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;
