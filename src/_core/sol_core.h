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

void Sol_MessageBox(const char *text, const char *level);
void Sol_Platform_LockCursor(bool lock);
void Sol_Platform_SetCursorpos(int x, int y);

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_End_Draw();

void Sol_State_SetTimescale(float timescale);