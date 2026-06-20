/*
 * File: sol_engine.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-19
 *
 */
#pragma once
#include "sol_core.h"

#define MAX_WORLDS 16

typedef struct World World;
typedef struct SolEngine
{
    volatile bool isRunning;
    volatile bool needsResize;
    int           windowWidth, windowHeight;
    int           windowX, windowY;
    void         *g_hwnd;

    u32    activeWorldId;
    World *activeWorld;
    World *hudWorld;
    World *worlds[MAX_WORLDS];
    u16    worldCount;
} SolEngine;
extern SolEngine solEngine;

int  Sol_Init(void *hwnd, void *hInstance);
void Sol_Tick(double dt, double time);
void Sol_Destroy();
void Sol_Window_OnResize(int x, int y, int width, int height);
void Sol_FPS(double dt);
void Sol_Debug_Draw(double dt);
void Sol_Interact_Update(World **world, int worldCount);
void Sol_Cam_Update(double dt);