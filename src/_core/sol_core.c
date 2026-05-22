#include "sol_core.h"

SolState solState = {0};

static double accumulator = 0.0;
static void   DebugFPS(double dt);
static void   Sol_OnResize();

void Sol_Init(void *hwnd, void *hInstance)
{
    solState.timescale = 1.0;
    solState.g_hwnd    = hwnd;
    i32 res;

    res = Sol_Audio_Init();
    if (res != 0)
        printf("Audio failed to init, code:%d\n", res);
    res = Sol_Audio_LoadAll();
    if (res != 0)
        printf("Audio failed to load, code:%d\n", res);
    res = Sol_Textures_Init();
    if (res != 0)
        printf("Textures failed to init, code:%d\n", res);
    res = Sol_Fonts_Init();
    if (res != 0)
        printf("Fonts failed to init, code:%d\n", res);
    res = Sol_Models_Init();
    if (res != 0)
        printf("Models failed to init, code:%d\n", res);
    res = Sol_Render_Init(hwnd, hInstance);
    if (res != 0)
        printf("Render failed to init, code:%d\n", res);

    solState.debug     = true;
    solState.isRunning = true;
}

SolState *Sol_GetState()
{
    return &solState;
}

double Sol_GetGameTime()
{
    return solState.gameTime;
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

void Sol_State_SetTimescale(float timescale)
{
    solState.timescale = timescale;
}

void Sol_State_SetActiveworld(World *world)
{
    solState.activeWorld = world;
}

void Sol_Tick(double dt, double time)
{
    solState.gameTime += dt;
    solState.tickCounter++;
    Sol_Input_Update();
    if (Sol_Input_KeyPressed(SOL_KEY_ESCAPE))
    {
        solState.worlds[0]->worldActive ^= 1;
        Sol_Input_SetLocked(!solState.worlds[0]->worldActive);
        // solState.isRunning = false;
    }

    if (solState.needsResize)
        Sol_OnResize();

    for (int i = 0; i < solState.worldCount; ++i)
    {
        if (!solState.worlds[i]->doesSimulate || !solState.worlds[i]->worldActive)
            continue;
        World_Tick(solState.worlds[i], dt, time);
    }

    // ######### STEP AND INTERP #########
    accumulator = accumulator > SOL_TIMESTEP * 10.0 ? SOL_TIMESTEP * 10.0 : accumulator + dt;
    for (int i = 0; i < solState.worldCount; ++i)
    {
        if (!solState.worlds[i]->doesSimulate || !solState.worlds[i]->worldActive)
            continue;
        if (accumulator >= SOL_TIMESTEP)
            Xform_Snapshot(solState.worlds[i]);
    }
    while (accumulator >= SOL_TIMESTEP)
    {
        for (int i = 0; i < solState.worldCount; ++i)
        {
            if (!solState.worlds[i]->doesSimulate || !solState.worlds[i]->worldActive)
                continue;
            World_Step(solState.worlds[i], SOL_TIMESTEP, time);
        }
        accumulator -= SOL_TIMESTEP;
    }
    float alpha = (float)(accumulator / SOL_TIMESTEP);
    for (int i = 0; i < solState.worldCount; ++i)
    {
        if (!solState.worlds[i]->doesSimulate || !solState.worlds[i]->worldActive)
            continue;
        Xform_Interpolate(solState.worlds[i], alpha);
    }
    // ######### END STEP AND INTERP #########

    if (solState.activeWorld)
        Sol_Audio_Update(Sol_Xform_GetPos(solState.activeWorld, solState.activeWorld->playerID), Sol_Controller_GetAimdir(solState.activeWorld, solState.activeWorld->playerID));

    Sol_Begin_Draw();

    for (int i = solState.worldCount - 1; i >= 0; --i)
    {
        if (!solState.worlds[i]->doesRender || !solState.worlds[i]->worldActive)
            continue;
        Sol_Cam_Update(solState.worlds[i], dt);
        World_Draw3d(solState.worlds[i], dt, time);
    }
    Sol_Render_DrawSkybox();
    Sol_Render_Flush3D();

    for (int i = solState.worldCount - 1; i >= 0; --i)
    {
        if (!solState.worlds[i]->doesRender || !solState.worlds[i]->worldActive)
            continue;
        World_Draw2d(solState.worlds[i], dt, time);
    }
    Sol_Render_Flush2D();

    Sol_Debug_Draw(dt);
    Sol_End_Draw();
}
