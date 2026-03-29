#include <stdatomic.h>
#include <string.h>

#include "input.h"

// maps SolKey to Windows virtual key codes
static int keyMap[SOL_KEY_COUNT] = {
    'W',
    'A',
    'S',
    'D',
};

static atomic_bool rawKeys[SOL_KEY_COUNT];
static atomic_bool rawMouseButtons[SOL_MOUSE_COUNT];
static atomic_int rawMouseX, rawMouseY;
static atomic_int rawMouseDeltaX, rawMouseDeltaY;

// snapshot read by game thread
static bool keys[SOL_KEY_COUNT];
static bool keysPrev[SOL_KEY_COUNT];
static bool mouseButtons[SOL_MOUSE_COUNT];
static bool mouseButtonsPrev[SOL_MOUSE_COUNT];
static int mouseX, mouseY;
static int mouseDeltaX, mouseDeltaY;

void SolInput_OnKey(int vkCode, bool down)
{
    for (int i = 0; i < SOL_KEY_COUNT; i++)
    {
        if (keyMap[i] == vkCode)
        {
            rawKeys[i] = down;
            return;
        }
    }
}

void SolInput_OnMouseMove(int x, int y)
{
    rawMouseX = x;
    rawMouseY = y;
}

void SolInput_OnMouseButton(SolMouseButton btn, bool down)
{
    rawMouseButtons[btn] = down;
}

SOLAPI void Sol_Input_OnRawMouse(int x, int y)
{
    rawMouseDeltaX += x;
    rawMouseDeltaY += y;
}

void SolInput_Update()
{
    // snapshot prev
    memcpy(keysPrev, keys, sizeof(keys));
    memcpy(mouseButtonsPrev, mouseButtons, sizeof(mouseButtons));

    // snapshot current
    for (int i = 0; i < SOL_KEY_COUNT; i++)
        keys[i] = rawKeys[i];
    for (int i = 0; i < SOL_MOUSE_COUNT; i++)
        mouseButtons[i] = rawMouseButtons[i];
    mouseX = rawMouseX;
    mouseY = rawMouseY;
    mouseDeltaX = atomic_exchange(&rawMouseDeltaX, 0);
    mouseDeltaY = atomic_exchange(&rawMouseDeltaY, 0);
}

bool SolInput_KeyDown(SolKey key)
{
    return keys[key];
}

bool SolInput_KeyPressed(SolKey key)
{
    return keys[key] && !keysPrev[key];
}

SolMouse SolInput_GetMouse()
{
    SolMouse m = {0};
    m.x = mouseX;
    m.y = mouseY;
    m.dx = mouseDeltaX;
    m.dy = mouseDeltaY;
    for (int i = 0; i < SOL_MOUSE_COUNT; i++)
    {
        m.buttons[i] = mouseButtons[i];
        m.buttonsPressed[i] = mouseButtons[i] && !mouseButtonsPrev[i];
    }
    return m;
}