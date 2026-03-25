#pragma once
#include <stdio.h>

#include "solmath.h"
#include "world.h"
#include "sol_load_types.h"

#include "render.h"
#include "singles/input.h"
#include "systems/controller/playercontroller.h"

typedef struct
{
    World **worlds;
    uint16_t worldCount;
} SolConfig;

void Sol_Init(void *hwnd, void *hInstance, SolConfig config);
void Sol_Tick(double dt, double time);
void Sol_Shutdown();
void Sol_Window_Resize(float width, float);

SolBank *Sol_Loader_GetBank(void);

void Sol_System_Button_Update(World *world, double dt, double time);