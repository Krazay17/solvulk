#include "sol_core.h"

#include "network.h"

SolState solState;

static double accumulator = SOL_TIMESTEP;
static void   DebugFPS(double dt);
static void   Sol_OnResize();

int Sol_Init(void *hwnd, void *hInstance)
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
    return res;
}

void Sol_Tick(double dt, double time)
{
    solState.gameTime += dt;
    solState.tickCounter++;
    Sol_Input_Update();

    if (solState.needsResize)
        Sol_OnResize();

    Sol_Net_Tick(solState.worlds, solState.worldCount);
    Worlds_Tick(solState.worlds, solState.worldCount, dt, time);

    // ######### STEP AND INTERP #########
    accumulator = accumulator > SOL_TIMESTEP * 10.0 ? SOL_TIMESTEP * 10.0 : accumulator + dt;
    while (accumulator >= SOL_TIMESTEP)
    {
        Sol_Interact_Update(solState.worlds, solState.worldCount);
        Sol_Xform_Snapshot(solState.worlds, solState.worldCount);
        Worlds_Step(solState.worlds, solState.worldCount, SOL_TIMESTEP, time);
        Sol_Net_Step(solState.worlds, solState.worldCount, time);
        Sol_Events_Clear(solState.worlds, solState.worldCount);
        solState.stepCounter++;
        accumulator -= SOL_TIMESTEP;
    }
    float alpha = (float)(accumulator / SOL_TIMESTEP);
    Sol_Xform_Interpolate(solState.worlds, solState.worldCount, alpha);
    // ######### END STEP AND INTERP #########

    if (solState.activeWorld)
        Sol_Audio_Update(Sol_Xform_GetPos(solState.activeWorld, 1), Sol_Cam_GetFwd());

    Sol_Begin_Draw();

    Sol_Cam_Update(dt);
    Sol_Render_DrawSkybox();

    Worlds_Draw3d(solState.worlds, solState.worldCount, dt, time);
    Sol_Render_Flush3D();

    Worlds_Draw2d(solState.worlds, solState.worldCount, dt, time);
    Sol_Tooltip_Draw();
    Sol_Render_Flush2D();

    Sol_Debug_Draw(dt);
    Sol_End_Draw();
}

World *Sol_GetWorldById(u32 id)
{
    return solState.worlds[id];
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
    Net_DeInit();

    for (int i = 0; i < solState.worldCount; i++)
    {
        free(solState.worlds[i]);
    }
    solState.isRunning = false;
}

static void Sol_OnResize()
{
    solState.needsResize = false;
    if (solState.windowWidth != 0 && solState.windowHeight != 0)
    {
        Sol_Render_Resize(solState.windowWidth, solState.windowHeight);
    }
}

void Sol_Window_OnResize(int x, int y, int width, int height)
{
    if (width <= 0 || height <= 0)
        return; // ignore degenerate (minimize)

    // Position (cheap, always update)
    solState.windowX = x;
    solState.windowY = y;

    // Size + derived values (skip if unchanged)
    if (width == solState.windowWidth && height == solState.windowHeight)
        return;

    solState.windowWidth  = width;
    solState.windowHeight = height;
    solState.aspectRatio  = (float)width / (float)height;

    // UI scale: fit logical (WINDOW_WIDTH × WINDOW_HEIGHT) inside actual window,
    // preserving aspect. min() = letterbox; max() = crop. Use min() for UI.
    float sx         = (float)width / WINDOW_WIDTH;
    float sy         = (float)height / WINDOW_HEIGHT;
    solState.uiScale = fminf(sx, sy);

    solState.needsResize = true; // game thread picks this up in Sol_OnResize
}

void Sol_State_SetTimescale(float timescale)
{
    solState.timescale = timescale;
}

World *Sol_State_GetActiveWorld()
{
    return solState.activeWorld;
}