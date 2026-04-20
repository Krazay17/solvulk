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

    World *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_Begin_3D();
SOLAPI void Sol_End_Draw();

SOLAPI void World_Step(World *world, double dt, double time);
SOLAPI void World_Tick(World *world, double dt, double time);
SOLAPI void World_Draw(World *world, double dt, double time);

// Xform systems
SOLAPI void Sol_System_Xform_Snapshot(World *world);
SOLAPI void Sol_System_Xform_Interpolate(World *world, float alpha);
// Step Systems
SOLAPI void Sol_System_Movement_2d_Step(World *world, double dt, double time);
SOLAPI void Sol_System_Movement_3d_Step(World *world, double dt, double time);
SOLAPI void Sol_System_Step_Physx_2d(World *world, double dt, double time);
SOLAPI void Sol_System_Step_Physx_3d(World *world, double dt, double time);
// Tick Systems
SOLAPI void Sol_System_Info_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Interact_Ui(World *world, double dt, double time);
SOLAPI void Sol_System_Controller_Local_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Controller_Ai_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Camera_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Line_Tick(World *world, double dt, double time);
SOLAPI void System_Combat_Tick(World *world, double dt, double time);
// Draw Systems
SOLAPI void Sol_System_Model_Draw(World *world, double dt, double time);
SOLAPI void Sol_System_Line_Draw(World *world, double dt, double time);
SOLAPI void Sol_System_UI_Draw(World *world, double dt, double time);
// Draw Calls
SOLAPI void Sol_Draw_Model_Instanced(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
SOLAPI void Sol_Draw_Rectangle(SolRect rect, SolColor color, float thickness);
SOLAPI void Sol_Draw_Line(SolLine *lines, int count);
SOLAPI void Sol_Draw_Text(const char *str, float x, float y, float size, SolColor color);