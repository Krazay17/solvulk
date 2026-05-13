/*
 * File: sol.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * ==== Sol Blade ====
 */
#pragma once
#include "sol/sol.h"

#include "profiler.h"
#include "event/event.h"

typedef struct SolState
{
    volatile bool isRunning;
    volatile bool needsResize;
    float         windowWidth, windowHeight;
    void         *g_hwnd;
    double        gameTime;
    double        fps;
    bool          debug;
    u32           tickCounter, stepCounter;

    World   *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;

void Sol_MessageBox(const char *text, const char *level);
void Sol_Platform_LockCursor(bool lock);

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_End_Draw();

