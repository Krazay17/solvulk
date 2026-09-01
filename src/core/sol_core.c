/*
 * File: sol_engine.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-19
 *
 */
#include "sol_core.h"
#include "sol_user.h"
#include "world.h"
#include "audio.h"
#include "input.h"
#include "image.h"
#include "model.h"
#include "font.h"
#include "render/render.h"

SolState solState;

static double accumulator = SOL_TIMESTEP;
static void   Sol_OnResize();

int Sol_Init(void *hwnd, void *hInstance)
{
    solState.timescale = 1.0;

    solState.g_hwnd = hwnd;
    int result;

    result = Sol_User_Init();
    if (result != 0)
        printf("User failed to init, code: %d\n", result);

    result = Sol_Audio_Init();
    if (result != 0)
        printf("Audio failed to init, code:%d\n", result);

    result = Sol_Render_Init(hwnd, hInstance);
    if (result != 0)
        printf("Render failed to init, code:%d\n", result);

    result = Sol_Audio_LoadAll();
    if (result != 0)
        printf("Audio failed to load, code:%d\n", result);

    result = Sol_Textures_Init();
    if (result != 0)
        printf("Texturesult failed to init, code:%d\n", result);

    result = Sol_Fonts_Init();
    if (result != 0)
        printf("Fonts failed to init, code:%d\n", result);

    result = Sol_Models_Init();
    if (result != 0)
        printf("Models failed to init, code:%d\n", result);

    solState.debug     = true;
    solState.isRunning = true;
    return result;
}

#define MAX_ACCUMULATOR (SOL_TIMESTEP * 5.0)
void Sol_Tick(double dt, double time)
{
    if (dt < 0.0 || dt > 1.0)
        dt = 0.0166666;
    solState.appTime = time;
    solState.tickCounter++;
    Sol_Input_Update();

    if (solState.needsResize)
        Sol_OnResize();

    // Sol_Net_Tick(solState.worlds, solState.worldCount);
    Sol_User_Tick(dt);
    Worlds_Tick(solState.worlds, solState.worldCount, dt);

    // ######### STEP AND INTERP #########
    accumulator = accumulator > MAX_ACCUMULATOR ? MAX_ACCUMULATOR : accumulator + dt;
    while (accumulator >= SOL_TIMESTEP)
    {
        Worlds_Xform_Snapshot(solState.worlds, solState.worldCount);
        Worlds_Step(solState.worlds, solState.worldCount, SOL_TIMESTEP);
        // Sol_Net_Step(solState.worlds, solState.worldCount, time);
        // Sol_Events_Clear(solState.worlds, solState.worldCount);
        solState.stepCounter++;
        accumulator -= SOL_TIMESTEP;
    }
    float alpha = (float)(accumulator / SOL_TIMESTEP);
    Worlds_Xform_Interpolate(solState.worlds, solState.worldCount, alpha);
    // ######### END STEP AND INTERP #########

    Worlds_PostTick(solState.worlds, solState.worldCount, dt);
    Sol_User_PostTick(dt);

    Sol_Update_Audio_FromView();
    Sol_Render_CheckGpuUploads();

    Sol_Begin_Draw();
    Sol_Render_DrawSkybox();
    Worlds_Draw3d(solState.worlds, solState.worldCount, dt);
    Sol_Render_Flush3D();

    Worlds_Draw2d(solState.worlds, solState.worldCount, dt);
    Sol_User_Draw(dt);
    Sol_Render_Flush2D();

    Sol_FPS(dt);
    Sol_Debug_Draw(dt);
    Sol_End_Draw();
}

void Sol_Destroy()
{
    // Net_DeInit();

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
