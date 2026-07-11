#include "input.h"
#include "sol_engine.h"
#include "sol_math.h"
#include "world.h"
#include "platform/platform.h"

#define SOL_KEYCODE_SPACE 32
#define SOL_KEYCODE_ESCAPE 27
#define SOL_KEYCODE_SHIFT 16

static int keyMap[256] = {
    [48] = SOL_KEY_0,     [49] = SOL_KEY_1,      [50] = SOL_KEY_2,     [51] = SOL_KEY_3,    [52] = SOL_KEY_4,
    [53] = SOL_KEY_5,     [54] = SOL_KEY_6,      [55] = SOL_KEY_7,     [56] = SOL_KEY_8,    [57] = SOL_KEY_9,
    ['W'] = SOL_KEY_W,    ['A'] = SOL_KEY_A,     ['S'] = SOL_KEY_S,    ['D'] = SOL_KEY_D,   ['F'] = SOL_KEY_F,
    [32] = SOL_KEY_SPACE, [27] = SOL_KEY_ESCAPE, [16] = SOL_KEY_SHIFT, [17] = SOL_KEY_CTRL, [18] = SOL_KEY_ALT,
    [81] = SOL_KEY_Q,     [69] = SOL_KEY_E,
};

static bool         rawKeys[SOL_KEY_COUNT];
static bool         rawMouseButtons[SOL_MOUSE_COUNT];
static volatile int rawMouseWheelDelta;
static volatile int rawMouseX, rawMouseY;
static volatile int rawMouseDeltaX, rawMouseDeltaY;

// snapshot read by game thread
static bool mouseLocked, toggleLocked;
static bool keys[SOL_KEY_COUNT];
static bool keysPrev[SOL_KEY_COUNT];
static bool mouseButtons[SOL_MOUSE_COUNT];
static bool mouseButtonsPrev[SOL_MOUSE_COUNT];
static int  mouseWheelDelta;
static int  mouseX, mouseY;
static int  mouseDeltaX, mouseDeltaY;

float       input_sens = 0.001f;
float       input_yaw;
float       input_pitch;

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

void Sol_Input_OnMouseButton(int btn, bool down)
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
    Sol_Debug_Add("MouseX", Sol_Input_GetMouseUI().x);
    Sol_Debug_Add("MouseY", Sol_Input_GetMouseUI().y);
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

    if (Sol_Input_KeyPressed(SOL_KEY_ESCAPE))
    {
        bool menuActive = solEngine.worlds[2]->doesSimulate;
        menuActive ^= 1;
        solEngine.worlds[2]->doesSimulate = menuActive;
        solEngine.worlds[2]->doesRender   = menuActive;
        Sol_Input_SetLocked(!menuActive);
    }

    if (Sol_Input_KeyPressed(SOL_KEY_ALT))
        Sol_Input_SetLocked(!toggleLocked);

    if (mouseButtons[SOL_MOUSE_RIGHT] || toggleLocked)
        mouseLocked = true;
    else
        mouseLocked = false;
    Sol_Platform_LockCursor(mouseLocked);

    if (mouseLocked)
    {
        input_yaw -= mouseDeltaX * input_sens;
        input_pitch -= mouseDeltaY * input_sens;

        input_yaw = fmodf(input_yaw, 2.0f * GLM_PIf);
        if (input_yaw > GLM_PIf)
            input_yaw -= 2.0f * GLM_PIf;
        else if (input_yaw < -GLM_PIf)
            input_yaw += 2.0f * GLM_PIf;

        if (input_pitch > MAX_PITCH)
            input_pitch = MAX_PITCH;
        if (input_pitch < -MAX_PITCH)
            input_pitch = -MAX_PITCH;
    }
}

vec3s Sol_Input_GetLookDir()
{
    return vecNorm(Sol_Vec3_FromYawPitch(input_yaw, input_pitch));
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

    m.dx     = mouseDeltaX;
    m.dy     = mouseDeltaY;
    m.wheelV = mouseWheelDelta;
    for (int i = 0; i < SOL_MOUSE_COUNT; i++)
    {
        m.buttons[i]         = mouseButtons[i];
        m.buttonsPressed[i]  = mouseButtons[i] && !mouseButtonsPrev[i];
        m.buttonsReleased[i] = mouseButtonsPrev[i] && !mouseButtons[i];
    }
    m.locked       = mouseLocked;
    m.togglelocked = toggleLocked;

    return m;
}

void Sol_Input_SetYawPitch(float y, float p)
{
    if (!isnan(y))
        input_yaw = y;
    if (!isnan(p))
        input_pitch = p;
}

void Sol_Input_SetLocked(bool lock)
{
    if (lock)
    {
        int width  = (int)((float)(solEngine.windowX + solEngine.windowWidth * 0.5f));
        int height = (int)((float)(solEngine.windowY + solEngine.windowHeight * 0.5f));
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

vec2s Sol_Input_GetMouseUI(void)
{
    SolMouse m = Sol_Input_GetMouse();
    return (vec2s){UIUNSCALE(m.x), UIUNSCALE(m.y)};
}