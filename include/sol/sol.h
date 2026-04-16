#pragma once

#include "sol/types.h"
#include "sol/world.h"
#include "sol/profiler.h"

#define SOL_VERSION 1.0

#define MAX_WORLDS 4

#define SOL_PHYS_GRAV (vec3s){0.0f, -9.81f, 0.0f}
#define SOL_PHYS_SUBSTEP 4
#define SOL_PHYS_TIMESTEP 1.0f / 60.0f

SOLAPI void Sol_Init(void *hwnd, void *hInstance);
SOLAPI void Sol_Tick(double dt, double time);
SOLAPI void Sol_Destroy();

SOLAPI SolState *Sol_GetState();
SOLAPI SolCamera *Sol_GetCamera();
SOLAPI double Sol_GetGameTime();

SOLAPI int Sol_Init_Vulkan(void *hwnd, void *hInstance);

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_Begin_3D();
SOLAPI void Sol_End_Draw();

SOLAPI void Sol_Render_Resize();
SOLAPI void Sol_SetOrtho(uint32_t width, uint32_t height);
SOLAPI void Sol_Camera_Update(vec3 pos, vec3 target);

SOLAPI void Sol_Draw_Model_Instanced(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
SOLAPI void Sol_Draw_Rectangle(SolRect rect, SolColor color, float thickness);

SOLAPI void Sol_Draw_Text(const char *str, float x, float y, float size, SolColor color);
SOLAPI float Sol_MeasureText(const char *str, float size);

SOLAPI void Sol_Window_Resize(float width, float height);
SOLAPI SolModel *Sol_GetModel(SolModelId id);
SOLAPI void Sol_Physics_BuildWorldCollider(World *world, SolModel *model);

// Needs free
SOLAPI char *Sol_ReadFile(const char *filename, long *outSize);

SOLAPI void Sol_FreeModel(SolModel *model);
SOLAPI void Sol_Loader_LoadModels();

SOLAPI void Sol_Debug_Add(const char *text, float value);

// called from WindowProc on main thread
SOLAPI void SolInput_OnKey(int vkCode, bool down);
SOLAPI void SolInput_OnMouseMove(int x, int y);
SOLAPI void SolInput_OnMouseButton(SolMouseButton btn, bool down);
SOLAPI void Sol_Input_OnRawMouse(int x, int y);

// called at start of each game frame to snapshot state
SOLAPI void SolInput_Update();

// query from game code
SOLAPI bool SolInput_KeyDown(SolKey key);
SOLAPI bool SolInput_KeyPressed(SolKey key); // true only on frame of press
SOLAPI SolMouse SolInput_GetMouse();