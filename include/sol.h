#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdatomic.h>

#include "solmath.h"
#include "world.h"
#include "loader.h"
#include "render.h"
#include "input.h"

#include "controller/playercontroller.h"
#include "ui/sol_ui.h"

#ifdef SOL_STATIC
    #define SOLAPI // Just empty for static linking
#else
    #ifdef SOL_BUILD_DLL
        #define SOLAPI __declspec(dllexport)
    #else
        #define SOLAPI __declspec(dllimport)
    #endif
#endif

typedef struct
{
    World **worlds;
    uint16_t worldCount;
} SolConfig;

typedef struct
{
    atomic_bool isRunning;
    atomic_bool needsResize;
    float windowWidth, windowHeight;
    void *g_hwnd;
} SolState;

extern SolState solState;

typedef struct
{
    const void *data;
    size_t size;
} SolResource;

typedef struct
{
    float position[3];
    float normal[3];
    float uv[2];
} SolVertex;

typedef struct
{
    SolVertex *vertices;
    uint32_t vertexCount;
    uint32_t *indices;
    uint32_t indexCount;
} SolMesh;

typedef struct SolModel
{
    SolMesh *meshes;
    uint32_t meshCount;
} SolModel;

SOLAPI void Sol_Init(void *hwnd, void *hInstance, SolConfig config);
SOLAPI void Sol_Tick(double dt, double time);
SOLAPI void Sol_Shutdown();

SOLAPI void Sol_Window_Resize(float width, float);
SOLAPI SolBank *Sol_Loader_GetBank(void);

SOLAPI void Sol_System_Button_Update(World *world, double dt, double time);
SOLAPI void Sol_System_Interact_Ui(World *world, double dt, double time);
SOLAPI void Sol_System_Update_View(World *world, double dt, double time);
SOLAPI void Sol_System_Step_Physx_2d(World *world, double dt, double time);
SOLAPI void Sol_System_Step_Physx_3d(World *world, double dt, double time);
SOLAPI void Sol_System_Info_Tick(World *world, double dt, double time);


// Needs free
SOLAPI char *Sol_ReadFile(const char *filename, size_t *outSize);
// No free needed - memory is owned by the exe
SOLAPI SolResource Sol_LoadResource(const char *resourceName);

// loads from embedded resource
SOLAPI SolModel Sol_LoadModel(const char *resourceName);
SOLAPI void Sol_FreeModel(SolModel *model);
SOLAPI void Sol_Loader_LoadModels();

SOLAPI int Sol_Prefab_Button(World *world, vec3s pos);
SOLAPI int Sol_Prefab_Wizard(World *world, vec3s pos);
