#include "sol_core.h"

#define SOL_KEYCODE_SPACE 32
#define SOL_KEYCODE_ESCAPE 27
#define SOL_KEYCODE_SHIFT 16

LocalInput local_input = {0};

// maps SolKey to Windows virtual key codes
static int keyMap[SOL_KEY_COUNT] = {
   48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 'W', 'A', 'S', 'D', 'F', SOL_KEYCODE_SPACE, SOL_KEYCODE_ESCAPE, SOL_KEYCODE_SHIFT,
};

static volatile bool  rawKeys[SOL_KEY_COUNT];
static volatile bool  rawMouseButtons[SOL_MOUSE_COUNT];
static volatile float rawMouseWheelDelta;
static volatile int   rawMouseX, rawMouseY;
static volatile int   rawMouseDeltaX, rawMouseDeltaY;

// snapshot read by game thread
static bool  mouseLocked;
static bool  keys[SOL_KEY_COUNT];
static bool  keysPrev[SOL_KEY_COUNT];
static bool  mouseButtons[SOL_MOUSE_COUNT];
static bool  mouseButtonsPrev[SOL_MOUSE_COUNT];
static float mouseWheelDelta;
static int   mouseX, mouseY;
static int   mouseDeltaX, mouseDeltaY;

void Sol_Input_OnKey(int vkCode, bool down)
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

void Sol_Input_OnMouseMove(int x, int y)
{
    rawMouseX = x;
    rawMouseY = y;
}

void Sol_Input_OnMouseButton(SolMouseButton btn, bool down)
{
    rawMouseButtons[btn] = down;
}

void Sol_Input_OnMouseWheel(int delta)
{
    rawMouseWheelDelta += delta;
}

void Sol_Input_OnRawMouse(int x, int y)
{
    rawMouseDeltaX += x;
    rawMouseDeltaY += y;
}

void Sol_Input_Update()
{
    // snapshot prev
    memcpy(keysPrev, keys, sizeof(keys));
    memcpy(mouseButtonsPrev, mouseButtons, sizeof(mouseButtons));

    // snapshot current
    for (int i = 0; i < SOL_KEY_COUNT; i++)
        keys[i] = rawKeys[i];
    for (int i = 0; i < SOL_MOUSE_COUNT; i++)
        mouseButtons[i] = rawMouseButtons[i];

    mouseX             = rawMouseX;
    mouseY             = rawMouseY;
    mouseDeltaX        = rawMouseDeltaX;
    rawMouseDeltaX     = 0;
    mouseDeltaY        = rawMouseDeltaY;
    rawMouseDeltaY     = 0;
    mouseWheelDelta    = rawMouseWheelDelta;
    rawMouseWheelDelta = 0;

    memcpy(local_input.keys, rawKeys, sizeof(bool) * SOL_KEY_COUNT);

    if (mouseButtons[SOL_MOUSE_RIGHT])
        mouseLocked = true;
    else
        mouseLocked = false;
    Sol_Platform_LockCursor(mouseLocked);
}

bool Sol_Input_KeyDown(SolKey key)
{
    return keys[key];
}

bool Sol_Input_KeyPressed(SolKey key)
{
    return keys[key] && !keysPrev[key];
}

SolMouse Sol_Input_GetMouse()
{
    SolMouse m = {0};
    m.x        = mouseX;
    m.y        = mouseY;
    m.dx       = mouseDeltaX;
    m.dy       = mouseDeltaY;
    m.wheelV   = mouseWheelDelta;
    for (int i = 0; i < SOL_MOUSE_COUNT; i++)
    {
        m.buttons[i]        = mouseButtons[i];
        m.buttonsPressed[i] = mouseButtons[i] && !mouseButtonsPrev[i];
    }
    m.locked = mouseLocked;

    return m;
}