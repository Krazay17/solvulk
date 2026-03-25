#include <windows.h>
#include <stdio.h>

#include "sol_core.h"
#include "sol.h"

SolState solState = {0};
static World **worlds;
static uint16_t g_worldCount;
static double timeStep = 1.0f / 120.0f;
static float smoothedFps = 120.0f;
static float accumulator = 0.0f;

static void DebugFPS(double dt);
static void Sol_OnResize();

void Sol_Init(void *hwnd, void *hInstance, SolConfig config)
{
    solState.g_hwnd = hwnd;
    g_worldCount = config.worldCount;
    worlds = config.worlds;
    int vulkInit = Sol_Init_Vulkan(hwnd, hInstance);
    printf("Vulkan Init code: %d\n", vulkInit);
    Sol_Loader_LoadModels();
}

void Sol_Tick(double dt, double time)
{
    SolInput_Update();
    if (SolInput_KeyPressed(SOL_KEY_ESCAPE))
        worlds[0]->worldActive ^= 1;

    if (solState.needsResize)
        Sol_OnResize();

    accumulator = accumulator > 0.25f ? 0.25f : accumulator + dt;
    while (accumulator >= timeStep)
    {
        for (int i = 0; i < g_worldCount; ++i)
            World_Step(worlds[i], timeStep, time);
        accumulator -= timeStep;
    }
    for (int i = 0; i < g_worldCount; ++i)
        World_Tick(worlds[i], dt, time);

    Sol_Begin_Draw();

    for (int i = g_worldCount - 1; i >= 0; --i)
        World_Draw(worlds[i], dt, time);

    DebugFPS(dt);

    Sol_End_Draw();
}

static void DebugFPS(double dt)
{
    float currentFps = dt > 0 ? 1.0f / (float)dt : 0.16;
    float alpha = 0.01f;
    smoothedFps = (alpha * currentFps) + (1.0f - alpha) * smoothedFps;
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Fps: %.0f", smoothedFps);
    Sol_Draw_Text(buffer, 0, 24.0f, 24.0f, (SolColor){0, 255, 0, 255});
}

void Sol_Shutdown()
{
    PostMessage(solState.g_hwnd, WM_CLOSE, 0, 0);
}

void Sol_Window_Resize(float width, float height)
{
    solState.windowWidth = width;
    solState.windowHeight = height;
    solState.needsResize = true;
}

static void Sol_OnResize()
{
    solState.needsResize = false;
    if (solState.windowWidth != 0 && solState.windowHeight != 0)
    {
        Sol_Render_Resize();
    }
}