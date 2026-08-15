#pragma once
#include "sol/types.h"

#define MAX_PITCH (GLM_PI_2f - 0.01f)

typedef struct SolMouse
{
    int  x, y;
    int  dx, dy;
    int  wheelV;
    bool locked, togglelocked;
    bool buttons[SOL_MOUSE_COUNT];
    bool buttonsPressed[SOL_MOUSE_COUNT];
    bool buttonsReleased[SOL_MOUSE_COUNT];
} SolMouse;

void     Sol_Input_OnKey(int vkCode, bool down);
void     Sol_Input_OnMouseMove(int x, int y);
void     Sol_Input_OnMouseButton(int btn, bool down);
void     Sol_Input_OnMouseWheel(int delta);
void     Sol_Input_OnRawMouse(int x, int y);
void     Sol_Input_Update();
bool     Sol_Input_KeyDown(SolKey key);
bool     Sol_Input_KeyPressed(SolKey key);
SolMouse Sol_Input_GetMouse();
void     Sol_Input_SetLocked(bool lock);
void     Sol_Input_Clear();
vec2s    Sol_Input_GetMouseUI(void);