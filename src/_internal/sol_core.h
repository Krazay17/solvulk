#pragma once

#include <sol/sol.h>

#include "debug_i.h"
#include "input_i.h"
#include "loader_i.h"
#include "math_i.h"
#include "movement_i.h"
#include "platform_i.h"
#include "profiler_i.h"
#include "render_i.h"

#include "systems/buff/buff_i.h"
#include "systems/combat/combat_i.h"
#include "systems/physx/physx_i.h"
#include "systems/view/view_i.h"
#include "systems/vital/vital_i.h"
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

    SingleFunc stepSystems[MAX_SYSTEMS];
    u32        stepCount;
    SingleFunc tickSystems[MAX_SYSTEMS];
    u32        tickCount;
    SingleFunc drawSystems[MAX_SYSTEMS];
    u32        drawCount;

    World   *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_Begin_3D();
SOLAPI void Sol_End_Draw();
void        Single_System_Add(SolState *state, SingleSystem system);

SOLAPI void World_Step(World *world, double dt, double time);
SOLAPI void World_Tick(World *world, double dt, double time);
SOLAPI void World_Draw(World *world, double dt, double time);

void Crosshair_Draw(double dt, double time);

static SingleConfig single_systems[SINGLE_SYS_COUNT] = {
    [SINGLE_SYS_CAM] = {.draw = Crosshair_Draw},
};
