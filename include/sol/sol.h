#pragma once

#include "sol/types.h"
#include "sol/world.h"

#define SOL_VERSION 1.0

#define SOL_PHYS_GRAV (vec3s){0.0f, -9.81f, 0.0f}
#define SOL_PHYS_SUBSTEP 4
#define SOL_PHYS_TIMESTEP 1.0f / 60.0f

SOLAPI void Sol_Init(void *hwnd, void *hInstance);
SOLAPI void Sol_Tick(double dt, double time);
SOLAPI void Sol_Destroy();

SOLAPI SolState  *Sol_GetState();
SOLAPI SolCamera *Sol_GetCamera();
SOLAPI double     Sol_GetGameTime();

SOLAPI int Sol_Init_Vulkan(void *hwnd, void *hInstance);
int        Sol_Init_Vulkan_Resources();

SOLAPI void Sol_Render_Resize();
SOLAPI void Sol_SetOrtho(uint32_t width, uint32_t height);

SOLAPI float Sol_MeasureText(const char *str, float size, SolFontId id);

SOLAPI void      Sol_Window_Resize(float width, float height);
SOLAPI SolModel *Sol_GetModel(SolModelId id);

// Needs free
SOLAPI char *Sol_ReadFile(const char *filename, long *outSize);

SOLAPI void Sol_FreeModel(SolModel *model);
SOLAPI void Sol_Loader_LoadModels();

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

SOLAPI void Sol_Debug_Add(const char *text, float value);

SolRayResult Sol_Raycast(World *world, SolRay ray);
SolRayResult Sol_RaycastD(World *world, SolRay ray, float debugDuration);

void Physx_Mass_Set(World *world, int id, float mass);