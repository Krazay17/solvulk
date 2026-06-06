/*
 * File: sol.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * ==== Sol Blade ====
 */
#pragma once
#include "sol/sol.h"

#include "event/event.h"
#include "network.h"
#include "profiler.h"

#define SOL_VERSION 1
#define SOL_TIMESTEP (1.0 / 60.0)
#define BITC(x) (1u << (x))
#define MAX_WORLDS 16

typedef struct SolState
{
    volatile bool isRunning;
    volatile bool needsResize;
    int           windowWidth, windowHeight;
    int           windowX, windowY;
    void         *g_hwnd;
    double        gameTime, timescale, fps;
    bool          debug;
    u32           tickCounter, stepCounter;
    double        uiScale, aspectRatio;

    u32    activeWorldId;
    World *activeWorld;
    World *hudWorld;
    World *worlds[MAX_WORLDS];
    u16    worldCount;
} SolState;

extern SolState solState;
#define UIUNSCALE(v) ((v) / solState.uiScale)
#define UISCALE(v) ((v) * solState.uiScale)

void   Sol_State_SetTimescale(float timescale);
World *Sol_State_GetActiveWorld(void);
void   Sol_Window_OnGeometryChanged(int x, int y, int width, int height);