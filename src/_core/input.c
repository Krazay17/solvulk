#include "sol_core.h"

#include "input.h"

#define SOL_KEYCODE_SPACE 32
#define SOL_KEYCODE_ESCAPE 27
#define SOL_KEYCODE_SHIFT 16

// typedef struct
// {
//     bool  mouseLocked, toggleLocked;
//     bool  keys[SOL_KEY_COUNT];
//     bool  keysPrev[SOL_KEY_COUNT];
//     bool  mouseButtons[SOL_MOUSE_COUNT];
//     bool  mouseButtonsPrev[SOL_MOUSE_COUNT];
//     float mouseWheelDelta;
//     int   mouseX, mouseY;
//     int   mouseDeltaX, mouseDeltaY;
//     u32   action;
// } LocalInput;
// static LocalInput local_input;

static int keyMap[256] = {
    [48] = SOL_KEY_0,     [49] = SOL_KEY_1,      [50] = SOL_KEY_2,     [51] = SOL_KEY_3,    [52] = SOL_KEY_4,
    [53] = SOL_KEY_5,     [54] = SOL_KEY_6,      [55] = SOL_KEY_7,     [56] = SOL_KEY_8,    [57] = SOL_KEY_9,
    ['W'] = SOL_KEY_W,    ['A'] = SOL_KEY_A,     ['S'] = SOL_KEY_S,    ['D'] = SOL_KEY_D,   ['F'] = SOL_KEY_F,
    [32] = SOL_KEY_SPACE, [27] = SOL_KEY_ESCAPE, [16] = SOL_KEY_SHIFT, [17] = SOL_KEY_CTRL, [18] = SOL_KEY_ALT,
};

static volatile bool rawKeys[SOL_KEY_COUNT];
static volatile bool rawMouseButtons[SOL_MOUSE_COUNT];
static volatile int  rawMouseWheelDelta;
static volatile int  rawMouseX, rawMouseY;
static volatile int  rawMouseDeltaX, rawMouseDeltaY;

// snapshot read by game thread
static bool    mouseLocked, toggleLocked;
static bool    keys[SOL_KEY_COUNT];
static bool    keysPrev[SOL_KEY_COUNT];
static bool    mouseButtons[SOL_MOUSE_COUNT];
static bool    mouseButtonsPrev[SOL_MOUSE_COUNT];
static int     mouseWheelDelta;
static int     mouseX, mouseY;
static int     mouseDeltaX, mouseDeltaY;
static SolLook sol_look = {.sens = 0.001f};

void Sol_Input_Init()
{
}

void Sol_Input_OnKey(int vkCode, bool down)
{
    Sol_Debug_Add("KeyCode", (float)vkCode);
    if (vkCode < 0 || vkCode > 256)
        return;
    int solKey      = keyMap[vkCode];
    rawKeys[solKey] = down;
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

    mouseX = rawMouseX;
    mouseY = rawMouseY;

    mouseDeltaX    = rawMouseDeltaX;
    rawMouseDeltaX = 0;
    mouseDeltaY    = rawMouseDeltaY;
    rawMouseDeltaY = 0;

    mouseWheelDelta    = rawMouseWheelDelta;
    rawMouseWheelDelta = 0;

    if (Sol_Input_KeyPressed(SOL_KEY_ALT))
        Sol_Input_SetLocked(!toggleLocked);

    if (mouseButtons[SOL_MOUSE_RIGHT] || toggleLocked)
        mouseLocked = true;
    else
        mouseLocked = false;
    Sol_Platform_LockCursor(mouseLocked);

    if (mouseLocked)
    {
        sol_look.yaw -= mouseDeltaX * sol_look.sens;
        sol_look.pitch -= mouseDeltaY * sol_look.sens;

        sol_look.yaw = fmodf(sol_look.yaw, 2.0f * GLM_PIf);
        if (sol_look.yaw > GLM_PIf)
            sol_look.yaw -= 2.0f * GLM_PIf;
        else if (sol_look.yaw < -GLM_PIf)
            sol_look.yaw += 2.0f * GLM_PIf;

        if (sol_look.pitch > MAX_PITCH)
            sol_look.pitch = MAX_PITCH;
        if (sol_look.pitch < -MAX_PITCH)
            sol_look.pitch = -MAX_PITCH;
    }
    sol_look.lookdir = glms_normalize(Sol_Vec3_FromYawPitch(sol_look.yaw, sol_look.pitch));
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
    m.locked       = mouseLocked;
    m.togglelocked = toggleLocked;

    return m;
}

void Sol_Input_SetYawPitch(float y, float p)
{
    if (!isnan(y))
        sol_look.yaw = y;
    if (!isnan(p))
        sol_look.pitch = p;
}

SolLook *Sol_Input_GetLook()
{
    return &sol_look;
}

void Sol_Input_SetLocked(bool lock)
{
    if (lock)
    {
        int width  = (int)((float)(Sol_GetState()->windowX + Sol_GetState()->windowWidth * 0.5f));
        int height = (int)((float)(Sol_GetState()->windowY + Sol_GetState()->windowHeight * 0.5f));
        Sol_Platform_SetCursorpos(width, height);
    }

    toggleLocked = lock;
}

void Sol_Input_Clear()
{
    memset(&rawKeys, 0, sizeof(rawKeys));
    memset(&rawMouseButtons, 0, sizeof(rawMouseButtons));
    memset(&keys, 0, sizeof(keys));
    memset(&mouseButtons, 0, sizeof(mouseButtons));
}