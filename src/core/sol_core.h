/*
 * File: sol_core.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-19
 *
 */
#pragma once
#include "sol/types.h"

#define SOL_TIMESTEP (1.0 / 60.0)

typedef enum
{
    WORLDID_SETTINGS,
    WORLDID_HUD,
    WORLDID_GAME2D,
    WORLDID_GAME3D_1,
    WORLDID_GAME3D_2,
    WORLDID_COUNT,
} WorldIdx;

typedef struct SolState
{
    World *worlds[WORLDID_COUNT];
    u16    worldCount;

    volatile bool isRunning;
    volatile bool needsResize;
    bool          debug;

    double appTime, timescale, fps;
    double uiScale, aspectRatio;
    u32    tickCounter, stepCounter;

    int   windowWidth, windowHeight;
    int   windowX, windowY;
    void *g_hwnd;
} SolState;

extern SolState solState;

#define UIUNSCALE(v) ((v) / solState.uiScale)
#define UISCALE(v) ((v) * solState.uiScale)

// Doubles capacity if count bigger than cap
static inline int Sol_Realloc(void **data, int count, int *capacity, size_t size)
{
    if (count >= *capacity)
    {
        int   newCap = (*capacity <= 0) ? 32 : *capacity * 2;
        void *tmp    = realloc(*data, size * newCap);
        if (!tmp)
        {
            fprintf(stderr, "Failed to realloc\n");
            return 1;
        }
        *data     = tmp;
        *capacity = newCap;
    }
    return 0;
}

static inline void Sol_ToggleDebug(int flag)
{
    solState.debug = !solState.debug;
}

static inline World *Sol_GetWorldByIdx(WorldIdx idx)
{
    return solState.worlds[idx];
}

int  Sol_Init(void *hwnd, void *hInstance);
void Sol_Tick(double dt, double time);
void Sol_Destroy();

void Sol_Window_OnResize(int x, int y, int width, int height);

void Sol_Debug_Add(const char *text, float value);
void Sol_Debug_Draw(double dt);
void Sol_FPS(double dt);