#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <assert.h>

#include "sol/platform.h"
#include "sol/common.h"
#include "sol/math.h"
#include "sol/world.h"
#include "sol/loader.h"
#include "sol/render.h"
#include "sol/input.h"
#include "sol/debug.h"
#include "sol/movement.h"

#define SOL_VERSION 1.0

#define MAX_WORLDS 4
#define SOL_PHYS_SUBSTEP 4
#define SOL_PHYS_TIMESTEP 1.0f / 60.0f

typedef enum
{
    ACTION_NONE = 0,
    ACTION_FWD = (1 << 0),
    ACTION_BWD = (1 << 1),
    ACTION_LEFT = (1 << 2),
    ACTION_RIGHT = (1 << 3),
    ACTION_JUMP = (1 << 4),
    ACTION_DASH = (1 << 5),
} PlayerActionStates;

typedef struct
{
    atomic_bool isRunning;
    atomic_bool needsResize;
    float windowWidth, windowHeight;
    void *g_hwnd;
    double gameTime;

    World *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;

typedef struct
{
    const void *data;
    size_t size;
} SolResource;

extern SolState solState;

SOLAPI void Sol_Init(void *hwnd, void *hInstance);
SOLAPI void Sol_Tick(double dt, double time);
SOLAPI void Sol_Destroy();

SOLAPI void Sol_Window_Resize(float width, float height);
SOLAPI SolBank *Sol_Loader_GetBank(void);

// Needs free
SOLAPI char *Sol_ReadFile(const char *filename, size_t *outSize);
// No free needed - memory is owned by the exe
SOLAPI SolResource Sol_LoadResource(const char *resourceName);

// loads from embedded resource
SOLAPI SolModel Sol_LoadModel(const char *resourceName);
SOLAPI void Sol_FreeModel(SolModel *model);
SOLAPI void Sol_Loader_LoadModels();
