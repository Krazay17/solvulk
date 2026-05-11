/*
 * File: sol.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * ==== Sol Blade ====
 */

#pragma once

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sol/types.h"
#include "world.h"

#define SOL_VERSION 1.0

#define MAX_SYSTEMS 64
#define MAX_ENTS (1 << 15)

SOLAPI void Sol_Init(void *hwnd, void *hInstance);
SOLAPI void Sol_Tick(double dt, double time);
SOLAPI void Sol_Destroy();

SOLAPI SolState *Sol_GetState();
SOLAPI double    Sol_GetGameTime();

SOLAPI float Sol_MeasureText(const char *str, float size, SolFontKind id);

SOLAPI void Sol_Window_Resize(float width, float height);

// called from WindowProc on main thread
SOLAPI void Sol_Input_OnKey(int vkCode, bool down);
SOLAPI void Sol_Input_OnMouseMove(int x, int y);
SOLAPI void Sol_Input_OnMouseButton(SolMouseButton btn, bool down);
SOLAPI void Sol_Input_OnMouseWheel(int delta);
SOLAPI void Sol_Input_OnRawMouse(int x, int y);

// called at start of each game frame to snapshot state
SOLAPI void Sol_Input_Update();

// query from game code
SOLAPI bool     Sol_Input_KeyDown(SolKey key);
SOLAPI bool     Sol_Input_KeyPressed(SolKey key); // true only on frame of press
SOLAPI SolMouse Sol_Input_GetMouse();
SOLAPI SolLook *Sol_Input_GetLook();

SOLAPI void Sol_Debug_Add(const char *text, float value);

SolRayResult Sol_Raycast(World *world, SolRay ray);
SolRayResult Sol_RaycastD(World *world, SolRay ray, float debugDuration);

typedef enum
{
    SOL_AUDIO_BEEP1,
    SOL_AUDIO_BEEP2,
    SOL_AUDIO_DIGILOAD,
    SOL_AUDIO_HIT,
    SOL_AUDIO_MENUMUSIC,
    SOL_AUDIO_SPACEGUN,
    SOL_AUDIO_WOONG,
    SOL_AUDIO_COUNT,
} SolAudioId;

void Sol_PlayAudio(SolAudioId id);
void Sol_Audio_PlayAt(SolAudioId id, vec3s pos);

static inline int Sol_Realloc(void **data, int count, int *capacity, size_t size)
{
    if (count >= *capacity)
    {
        int   newCap = (*capacity <= 0) ? 128 : *capacity * 2;
        void *tmp    = realloc(*data, size * newCap);
        if (!tmp)
        {
            fprintf(stderr, "Failed to realloc");
            return 1;
        }
        *data     = tmp;
        *capacity = newCap;
    }
    return 0;
}