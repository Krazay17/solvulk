#include "sol_core.h"
#include "sol_bank.h"

SolState solState = {0};

static double accumulator = 0.0;
static void DebugFPS(double dt);
static void Sol_OnResize();

void Sol_Init(void *hwnd, void *hInstance)
{
    solState.g_hwnd    = hwnd;
    solState.isRunning = true;
    int vulkInit       = Sol_Init_Vulkan(hwnd, hInstance);
    printf("Vulkan Init code: %d\n", vulkInit);

    Sol_Load_Resources();
    Sol_Init_Vulkan_Resources();

    solState.debug = true;
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
    solState.tickCounter++;
    Sol_Input_Update();
    if (Sol_Input_KeyPressed(SOL_KEY_ESCAPE))
    {
        solState.debug = !solState.debug;
        solState.worlds[0]->worldActive ^= 1;
        // solState.isRunning = false;
    }

    if (solState.needsResize)
        Sol_OnResize();

    for (int i = 0; i < solState.singleCount; ++i)
        if (solState.singleSystems[i])
            solState.singleSystems[i](dt, time);
    for (int i = 0; i < solState.worldCount; ++i)
        World_Tick(solState.worlds[i], dt, time);

    // Step at limited timestep -------
    accumulator = accumulator > SOL_TIMESTEP * 10.0 ? SOL_TIMESTEP * 10.0 : accumulator + dt;
    if (accumulator >= SOL_TIMESTEP)
        for (int i = 0; i < solState.worldCount; ++i)
            Xform_Snapshot(solState.worlds[i]);

    while (accumulator >= SOL_TIMESTEP)
    {
        for (int i = 0; i < solState.worldCount; ++i)
        {
            World_Step(solState.worlds[i], SOL_TIMESTEP, time);
            Event_Clear(solState.worlds[i]);
        }

        solState.stepCounter++;
        accumulator -= SOL_TIMESTEP;
    }
    // ---------------------------------

    float alpha = (float)(accumulator / SOL_TIMESTEP);
    for (int i = 0; i < solState.worldCount; ++i)
    {
        Xform_Interpolate(solState.worlds[i], alpha);
        Cam_Update_3D(solState.worlds[i], dt, time, alpha);
    }

    Sol_Begin_Draw();
    Sol_Begin_3D();

    for (int i = solState.worldCount - 1; i >= 0; --i)
        World_Draw3d(solState.worlds[i], dt, time);
    for (int i = solState.worldCount - 1; i >= 0; --i)
        World_Draw2d(solState.worlds[i], dt, time);

    Sol_Debug_Draw(dt);
    Sol_End_Draw();
}

void Sol_Destroy()
{
    for (int i = 0; i < solState.worldCount; i++)
    {
        free(solState.worlds[i]);
    }
    solState.isRunning = false;
}

void Sol_Window_Resize(float width, float height)
{
    solState.windowWidth  = width;
    solState.windowHeight = height;
    solState.needsResize  = true;
}

static void Sol_OnResize()
{
    solState.needsResize = false;
    if (solState.windowWidth != 0 && solState.windowHeight != 0)
    {
        Sol_Render_Resize(solState.windowWidth, solState.windowHeight);
    }
}
