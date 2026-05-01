#pragma once

#include <sol/sol.h>

#include "debug.h"
#include "input.h"
#include "loader.h"
#include "sol_math.h"
#include "platform.h"
#include "profiler.h"

#include "render/render.h"
#include "buff/buff.h"
#include "combat/combat.h"
#include "physx/physx.h"
#include "view/view.h"
#include "vital/vital.h"
typedef void (*SingleFunc)(double dt, double time);

typedef enum
{
    SINGLE_SYS_CAM,
    SINGLE_SYS_COUNT,
} SingleSystem;

typedef struct SingleConfig
{
    SingleFunc step;
    SingleFunc tick;
    SingleFunc draw;
} SingleConfig;

typedef struct SolState
{
    volatile bool isRunning;
    volatile bool needsResize;
    float         windowWidth, windowHeight;
    void         *g_hwnd;
    double        gameTime;
    double        fps;
    bool          debug;
    u32           tickCounter, stepCounter;

    SingleFunc singleSystems[MAX_SYSTEMS];
    u32        singleCount;

    World   *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_Begin_3D();
SOLAPI void Sol_End_Draw();

SOLAPI void World_Step(World *world, double dt, double time);
SOLAPI void World_Tick(World *world, double dt, double time);
SOLAPI void World_Draw(World *world, double dt, double time);
