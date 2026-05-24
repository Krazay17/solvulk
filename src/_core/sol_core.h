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
#include "profiler.h"

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

    World   *activeWorld;
    World   *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;


void Sol_State_SetTimescale(float timescale);