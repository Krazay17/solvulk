#pragma once
#include <stdbool.h>

#include "sol_common.h"

typedef enum
{
    SOL_KEY_W,
    SOL_KEY_A,
    SOL_KEY_S,
    SOL_KEY_D,
    SOL_KEY_SPACE,
    SOL_KEY_ESCAPE,
    SOL_KEY_COUNT
} SolKey;

typedef enum
{
    SOL_MOUSE_LEFT,
    SOL_MOUSE_RIGHT,
    SOL_MOUSE_MIDDLE,
    SOL_MOUSE_COUNT
} SolMouseButton;

typedef struct
{
    int x, y;
    int dx, dy;
    bool buttons[SOL_MOUSE_COUNT];
    bool buttonsPressed[SOL_MOUSE_COUNT];
} SolMouse;

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