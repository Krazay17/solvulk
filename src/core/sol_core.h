/*
 * File: sol_core.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * ==== Sol Blade ====
 */
#pragma once
#include "sol/types.h"

#define SOL_TIMESTEP (1.0 / 60.0)

typedef struct World World;

typedef struct SolState
{
    double gameTime, timescale, fps;
    bool   debug;
    u32    tickCounter, stepCounter;
    double uiScale, aspectRatio;
} SolState;

extern SolState solState;
#define UIUNSCALE(v) ((v) / solState.uiScale)
#define UISCALE(v) ((v) * solState.uiScale)

void   Sol_Debug_Add(const char *text, float value);
World *Sol_GetActiveWorld(void);
vec3s  Sol_Cam_GetRight();
vec3s  Sol_Cam_GetFwd();
vec3s  Sol_Cam_GetPos();
mat4s  Sol_Cam_GetViewProj();

// Doubles capacity if count bigger than cap
static inline int Sol_Realloc(void **data, int count, int *capacity, size_t size)
{
    if (count >= *capacity)
    {
        int   newCap = (*capacity <= 0) ? 128 : *capacity * 2;
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