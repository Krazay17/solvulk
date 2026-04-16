#include "sol_core.h"

SolState solState = {0};

static double accumulator = 0.0;

static void DebugFPS(double dt);
static void Sol_OnResize();

void Sol_Init(void *hwnd, void *hInstance)
{
    solState.g_hwnd = hwnd;
    int vulkInit = Sol_Init_Vulkan(hwnd, hInstance);
    printf("Vulkan Init code: %d\n", vulkInit);
    Sol_Loader_LoadModels();
}

SolState *Sol_GetState()
{
    return &solState;
}

double Sol_GetGameTime()
{
    return solState.gameTime;
}

void Sol_Tick(double dt, double time)
{
    solState.gameTime = time;
    solState.tickCount++;
    SolInput_Update();
    if (SolInput_KeyPressed(SOL_KEY_ESCAPE))
        solState.worlds[0]->worldActive ^= 1;

    if (solState.needsResize)
        Sol_OnResize();

    for (int i = 0; i < solState.worldCount; ++i)
        World_Tick(solState.worlds[i], dt, time);

    for (int i = 0; i < solState.worldCount; ++i)
        Sol_System_Xform_Snapshot(solState.worlds[i]);

    accumulator = accumulator > SOL_TIMESTEP * 2 ? SOL_TIMESTEP * 2 : accumulator + dt;
    while (accumulator >= SOL_TIMESTEP)
    {
        accumulator -= SOL_TIMESTEP;
        solState.stepCount++;
        Sol_Debug_Add("StepCount", solState.stepCount);
        for (int i = 0; i < solState.worldCount; ++i)
            World_Step(solState.worlds[i], SOL_TIMESTEP, time);
    }
    float alpha = (float)(accumulator / SOL_TIMESTEP);
    for (int i = 0; i < solState.worldCount; ++i)
        Sol_System_Xform_Interpolate(solState.worlds[i], alpha);

    Sol_Begin_Draw();

    for (int i = solState.worldCount - 1; i >= 0; --i)
        World_Draw(solState.worlds[i], dt, time);

    Sol_Debug_Draw();

    DebugFPS(dt);

    Sol_End_Draw();
}

static void DebugFPS(double dt)
{
    static double total,throttle;
    static char buffer[64];
    static int count;
    total += 1.0 / dt;
    count++;

    if ((throttle += dt) > 0.1)
    {
        float currentFps = total / count;
        snprintf(buffer, sizeof(buffer), "Fps: %.0f", currentFps);
        throttle = 0;
        count = 0;
        total = 0;
    }
    Sol_Draw_Text(buffer, 6.0f, 24.0f, 24.0f, (SolColor){0, 255, 0, 255});
}

void Sol_Destroy()
{
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