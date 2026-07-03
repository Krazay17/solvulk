/*
 * File: sol_engine.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-19
 *
 */
#include "sol_engine.h"
#include "sol_core.h"
#include "world.h"
#include "network.h"
#include "audio.h"
#include "input.h"
#include "image.h"
#include "model.h"
#include "font.h"
#include "xform/s_xform.h"
#include "interact/s_interact.h"
#include "render/render.h"

SolEngine solEngine;
SolState  solState;

static double accumulator = SOL_TIMESTEP;
static void   Sol_OnResize();

int Sol_Init(void *hwnd, void *hInstance)
{
    solState.timescale = 1.0;
    solEngine.g_hwnd   = hwnd;
    int result;

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

    solState.debug      = true;
    solEngine.isRunning = true;
    return result;
}

void Sol_Tick(double dt, double time)
{
    sollog("Ticking");
    if (dt < 0.0 || dt > 1.0)
        dt = 0.0166666;
    solState.gameTime += dt;
    solState.tickCounter++;
    Sol_Input_Update();

    if (solEngine.needsResize)
        Sol_OnResize();

    Sol_Net_Tick(solEngine.worlds, solEngine.worldCount);
    Worlds_Tick(solEngine.worlds, solEngine.worldCount, dt, time);

    // ######### STEP AND INTERP #########
    accumulator = accumulator > SOL_TIMESTEP * 10.0 ? SOL_TIMESTEP * 10.0 : accumulator + dt;
    while (accumulator >= SOL_TIMESTEP)
    {
        Sol_Interact_Update(solEngine.worlds, solEngine.worldCount);
        Sol_Xform_Snapshot(solEngine.worlds, solEngine.worldCount);
        Worlds_Step(solEngine.worlds, solEngine.worldCount, SOL_TIMESTEP, time);
        Sol_Net_Step(solEngine.worlds, solEngine.worldCount, time);
        Sol_Events_Clear(solEngine.worlds, solEngine.worldCount);
        solState.stepCounter++;
        accumulator -= SOL_TIMESTEP;
    }
    float alpha = (float)(accumulator / SOL_TIMESTEP);
    Sol_Xform_Interpolate(solEngine.worlds, solEngine.worldCount, alpha);
    // ######### END STEP AND INTERP #########

    if (solEngine.activeWorld)
        Sol_Audio_Update(Sol_Xform_GetPos(solEngine.activeWorld, 1), Sol_Cam_GetFwd());

    Sol_Begin_Draw();

    Sol_Cam_Update(dt);
    Sol_Render_DrawSkybox();

    Worlds_Draw3d(solEngine.worlds, solEngine.worldCount, dt, time);
    Sol_Render_Flush3D();
    Worlds_Draw2d(solEngine.worlds, solEngine.worldCount, dt, time);
    Sol_Tooltip_Draw();
    Sol_Render_Flush2D();

    Sol_FPS(dt);
    Sol_Debug_Draw(dt);
    Sol_End_Draw();
}

World *Sol_GetWorldById(u32 id)
{
    return solEngine.worlds[id];
}

void Sol_Destroy()
{
    Net_DeInit();

    for (int i = 0; i < solEngine.worldCount; i++)
    {
        free(solEngine.worlds[i]);
    }
    solEngine.isRunning = false;
}

static void Sol_OnResize()
{
    solEngine.needsResize = false;
    if (solEngine.windowWidth != 0 && solEngine.windowHeight != 0)
    {
        Sol_Render_Resize(solEngine.windowWidth, solEngine.windowHeight);
    }
}

void Sol_Window_OnResize(int x, int y, int width, int height)
{
    if (width <= 0 || height <= 0)
        return; // ignore degenerate (minimize)

    // Position (cheap, always update)
    solEngine.windowX = x;
    solEngine.windowY = y;

    // Size + derived values (skip if unchanged)
    if (width == solEngine.windowWidth && height == solEngine.windowHeight)
        return;

    solEngine.windowWidth  = width;
    solEngine.windowHeight = height;
    solState.aspectRatio   = (float)width / (float)height;

    // UI scale: fit logical (WINDOW_WIDTH × WINDOW_HEIGHT) inside actual window,
    // preserving aspect. min() = letterbox; max() = crop. Use min() for UI.
    float sx         = (float)width / WINDOW_WIDTH;
    float sy         = (float)height / WINDOW_HEIGHT;
    solState.uiScale = fminf(sx, sy);

    solEngine.needsResize = true; // game thread picks this up in Sol_OnResize
}
World *Sol_GetActiveWorld()
{
    return solEngine.activeWorld;
}