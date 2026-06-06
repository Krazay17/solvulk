#pragma once
#include "sol/base.h"

#define MAX_PITCH (GLM_PI_2f - 0.01f)

typedef enum
{
    SOL_MOUSE_LEFT,
    SOL_MOUSE_RIGHT,
    SOL_MOUSE_MIDDLE,
    SOL_MOUSE_COUNT
} SolMouseButton;

typedef enum
{
    SOL_KEY_0,
    SOL_KEY_1,
    SOL_KEY_2,
    SOL_KEY_3,
    SOL_KEY_4,
    SOL_KEY_5,
    SOL_KEY_6,
    SOL_KEY_7,
    SOL_KEY_8,
    SOL_KEY_9,

    SOL_KEY_W,
    SOL_KEY_A,
    SOL_KEY_S,
    SOL_KEY_D,
    SOL_KEY_F,
    SOL_KEY_SPACE,
    SOL_KEY_ESCAPE,
    SOL_KEY_SHIFT,
    SOL_KEY_ALT,
    SOL_KEY_CTRL,
    SOL_KEY_COUNT
} SolKey;

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

typedef struct SolLook
{
    float yaw, pitch, sens;
    vec3s lookdir;
} SolLook;

void     Sol_Input_OnKey(int vkCode, bool down);
void     Sol_Input_OnMouseMove(int x, int y);
void     Sol_Input_OnMouseButton(SolMouseButton btn, bool down);
void     Sol_Input_OnMouseWheel(int delta);
void     Sol_Input_OnRawMouse(int x, int y);
void     Sol_Input_Update();
bool     Sol_Input_KeyDown(SolKey key);
bool     Sol_Input_KeyPressed(SolKey key); // true only on frame of press
SolMouse Sol_Input_GetMouse();
SolLook *Sol_Input_GetLook();
void     Sol_Input_SetLocked(bool lock);
void     Sol_Input_Clear();
vec2s    Sol_Input_GetMouseUI(void);