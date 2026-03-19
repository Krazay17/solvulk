#include <windows.h>
#include <stdio.h>

#include "world.h"
#include "loader.h"
#include "render.h"
#include "input.h"
#include "solglobals.h"

#define MAX_WORLDS 2

static World *worlds[MAX_WORLDS] = {0};
static float smoothedFps = 120.0f;
static float accumulator = 0.0f;
static float timeStep = 1.0f / 120.0f;

void Sol_Init(HWND hwnd, HINSTANCE hInstance)
{
    int vulkInit = Sol_Init_Vulkan(hwnd, hInstance);
    printf("Vulkan Init code: %d\n", vulkInit);
    LoadModels();

    worlds[0] = GetGame();
    for (int i = 0; i < MAX_WORLDS; ++i)
    {
        if (worlds[i] && worlds[i]->init)
            worlds[i]->init();
    }
}

void Sol_Tick(double dt, double time)
{
    SolInput_Update();
    
    accumulator += dt;
    if (accumulator > 0.25f)
        accumulator = 0.25f;
    while (accumulator >= timeStep)
    {
        for (int i = 0; i < MAX_WORLDS; ++i)
        {
            if (worlds[i] && worlds[i]->step)
                worlds[i]->step(dt, time);
        }
        accumulator -= timeStep;
    }
    for (int i = 0; i < MAX_WORLDS; ++i)
    {
        if (worlds[i] && worlds[i]->tick)
            worlds[i]->tick(dt, time);
    }

    Sol_Begin_Draw();

    for (int i = 0; i < MAX_WORLDS; ++i)
    {
        if (worlds[i] && worlds[i]->draw)
            worlds[i]->draw();
    }

    float currentFps = 1.0f / (float)dt;
    float alpha = 0.001f;
    smoothedFps = (alpha * currentFps) + (1.0f - alpha) * smoothedFps;
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Fps: %.0f", smoothedFps);
    Sol_Draw_Text(buffer, 0, 24.0f, 24.0f, (SolColor){0, 255, 0, 255});

    Sol_End_Draw();
}

void Sol_On_Resize()
{
    Sol_Render_Resize();
}