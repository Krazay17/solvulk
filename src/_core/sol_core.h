#pragma once
#include "sol/sol.h"

#include "profiler.h"
#include "world/world.h"
#include "resource/resource.h"
#include "maths/sol_math.h"
#include "render/render.h"

#define SOL_TIMESTEP (1.0 / 60.0)
#define MAX_WORLDS 4

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

    World   *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_Begin_3D(SolCamera *cam);
SOLAPI void Sol_End_Draw();

SOLAPI void World_Step(World *world, double dt, double time);
SOLAPI void World_Tick(World *world, double dt, double time);
SOLAPI void World_Draw3d(World *world, double dt, double time);
SOLAPI void World_Draw2d(World *world, double dt, double time);

extern const char *fontResourceName[SOL_FONT_COUNT][2];
void               Sol_Load_Resources();
SolResource        Sol_LoadResource(const char *resourceName);
int                Sol_ReadFile(const char *filename, SolResource *outRes);

void Sol_MessageBox(const char *text, const char *level);
void Sol_Platform_LockCursor(bool lock);