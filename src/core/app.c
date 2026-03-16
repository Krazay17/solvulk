#include "world.h"
#include "loader.h"
#include "render.h"
#include "input.h"

#include <windows.h>
#include <stdio.h>

#define MAX_WORLDS 2

static World *worlds[MAX_WORLDS] = {0};

void Sol_Init(HWND hwnd, HINSTANCE hInstance)
{
    Sol_Init_Vulkan(hwnd, hInstance);
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
    printf("%f :: %f\n", dt, time);

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