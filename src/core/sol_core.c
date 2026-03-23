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

void Sol_Init(void *hwnd, void *hInstance, SolConfig config)
{
    solState.g_hwnd = hwnd;
    g_worldCount = config.worldCount;
    worlds = config.worlds;
    int vulkInit = Sol_Init_Vulkan(hwnd, hInstance);
    printf("Vulkan Init code: %d\n", vulkInit);
    Sol_Loader_LoadModels();

    for (int i = 0; i < g_worldCount; ++i)
    {
        if (worlds[i] && worlds[i]->init)
            worlds[i]->init();
    }
}

void Sol_Tick(double dt, double time)
{
    SolInput_Update();
    if (SolInput_KeyPressed(SOL_KEY_ESCAPE))
    {
        // World *menu = Sol_GetMenu();
        // menu->active = !menu->active;
        worlds[0]->active ^= 1;
    }

    if (solState.needsResize)
    {
        Sol_Render_Resize();
        solState.needsResize = false;
    }

    accumulator += dt;
    if (accumulator > 0.25f)
        accumulator = 0.25f;
    while (accumulator >= timeStep)
    {
        for (int i = 0; i < g_worldCount; ++i)
        {
            if (worlds[i] && worlds[i]->active && worlds[i]->step)
                worlds[i]->step(timeStep, time);
        }
        accumulator -= timeStep;
    }
    for (int i = 0; i < g_worldCount; ++i)
    {
        if (worlds[i] && worlds[i]->active && worlds[i]->tick)
            worlds[i]->tick(dt, time);
    }

    Sol_Begin_Draw();

    for (int i = g_worldCount - 1; i >= 0; --i)
    {
        if (worlds[i] && worlds[i]->active && worlds[i]->draw)
            worlds[i]->draw();
    }

    float currentFps = dt > 0 ? 1.0f / (float)dt : 0.16;
    float alpha = 0.01f;
    smoothedFps = (alpha * currentFps) + (1.0f - alpha) * smoothedFps;
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Fps: %.0f", smoothedFps);
    Sol_Draw_Text(buffer, 0, 24.0f, 24.0f, (SolColor){0, 255, 0, 255});

    Sol_End_Draw();
}

void Sol_Shutdown()
{
    PostMessage(solState.g_hwnd, WM_CLOSE, 0, 0);
}