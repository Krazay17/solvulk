#include "sol_core.h"

#define SOL_KEYCODE_SPACE 32
#define SOL_KEYCODE_ESCAPE 27
#define SOL_KEYCODE_SHIFT 16

// maps SolKey to Windows virtual key codes
static int keyMap[SOL_KEY_COUNT] = {
    'W',
    'A',
    'S',
    'D',
    'F',
    SOL_KEYCODE_SPACE,
    SOL_KEYCODE_ESCAPE,
    SOL_KEYCODE_SHIFT,
};

static volatile bool rawKeys[SOL_KEY_COUNT];
static volatile bool rawMouseButtons[SOL_MOUSE_COUNT];
static volatile int rawMouseX, rawMouseY;
static volatile int rawMouseDeltaX, rawMouseDeltaY;

// snapshot read by game thread
static bool mouseLocked;
static bool keys[SOL_KEY_COUNT];
static bool keysPrev[SOL_KEY_COUNT];
static bool mouseButtons[SOL_MOUSE_COUNT];
static bool mouseButtonsPrev[SOL_MOUSE_COUNT];
static int mouseX, mouseY;
static int mouseDeltaX, mouseDeltaY;

void SolInput_OnKey(int vkCode, bool down)
{
    Sol_Debug_Add("KeyCode", vkCode);
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
    mouseDeltaX = rawMouseDeltaX;
    rawMouseDeltaX = 0;
    mouseDeltaY = rawMouseDeltaY;
    rawMouseDeltaY = 0;

    if (mouseButtons[SOL_MOUSE_RIGHT])
        mouseLocked = true;
    else
        mouseLocked = false;

    Sol_Platform_LockCursor(mouseLocked);
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
    m.locked = mouseLocked;
    return m;
}