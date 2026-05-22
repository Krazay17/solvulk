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

#include "input.h"
#include "resource.h"
#include "sol_math.h"

#include "world.h"
#include "components.h"

#include "model/model.h"
#include "texture/texture.h"
#include "owner/owner.h"
#include "combat/combat.h"
#include "contact/contact.h"
#include "buff/buff.h"
#include "view/view.h"
#include "render/render.h"
#include "parent/parent.h"
#include "font/font.h"
#include "ability/ability.h"
#include "audio/audio.h"
#include "camera/camera.h"
#include "controller/controller.h"
#include "emitter/emitter.h"
#include "interact/interact.h"
#include "line/line.h"
#include "movement/movement.h"
#include "physx/physx.h"
#include "ui/ui.h"
#include "vital/vital.h"
#include "xform/xform.h"

#include "game/prefabs.h"

#define SOL_VERSION 1.0

#define SOL_TIMESTEP (1.0 / 60.0)
#define MAX_WORLDS 4

#define UISCALE(x) (x * min((float)Sol_GetState()->windowWidth / WINDOW_WIDTH, (float)Sol_GetState()->windowHeight / WINDOW_HEIGHT))
#define ColorConvert(x) (x / 255.0f)

SOLAPI void      Sol_Init(void *hwnd, void *hInstance);
SOLAPI void      Sol_Tick(double dt, double time);
SOLAPI void      Sol_Destroy();
SOLAPI SolState *Sol_GetState();
void Sol_State_SetActiveworld(World *world);
SOLAPI double    Sol_GetGameTime();
SOLAPI void      Sol_Window_Resize(float width, float height);


void Sol_Debug_Add(const char *text, float value);
void DebugFPS(double dt);

// Doubles capacity if data bigger than cap
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