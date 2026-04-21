#pragma once

#include <sol/sol.h>

#include "math_i.h"
#include "movement_i.h"
#include "render_i.h"
#include "platform_i.h"
#include "loader_i.h"
#include "profiler_i.h"
#include "debug_i.h"

#include "systems/physx/physx_i.h"
#include "systems/view/view_i.h"

typedef struct SolState
{
    volatile bool isRunning;
    volatile bool needsResize;
    float windowWidth, windowHeight;
    void *g_hwnd;
    double gameTime;
    u64 stepCount, tickCount;
    double fps;
    bool debug;

    World *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_Begin_3D();
SOLAPI void Sol_End_Draw();

SOLAPI void World_Step(World *world, double dt, double time);
SOLAPI void World_Tick(World *world, double dt, double time);
SOLAPI void World_Draw(World *world, double dt, double time);