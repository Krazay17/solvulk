#include "world.h"
#include "button.h"
#include "loader.h"
#include "render.h"
#include "input.h"

#include <windows.h>
#include <stdio.h>

#define MAX_WORLDS 2

static World *worlds[MAX_WORLDS] = {0};
static float accumulator = 0.0f;
static float timeStep = 1 / 120;

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
    //printf("%f :: %f\n", dt, time);
    // accumulator += dt;
    // if (accumulator > 0.25f)
    //     accumulator = 0.25f;
    // while (accumulator >= timeStep)
    // {
    //     for (int i = 0; i < MAX_WORLDS; ++i)
    //     {
    //         if (worlds[i] && worlds[i]->step)
    //             worlds[i]->step(dt, time);
    //     }
    //     accumulator -= timeStep;
    // }
    for (int i = 0; i < MAX_WORLDS; ++i)
    {
        if (worlds[i] && worlds[i]->tick)
            worlds[i]->tick(dt, time);
    }
    for (int i = 0; i < MAX_WORLDS; ++i)
    {
        if (worlds[i] && worlds[i]->draw)
            worlds[i]->draw();
    }
}

void Sol_On_Resize()
{
    Sol_Render_Resize();
}