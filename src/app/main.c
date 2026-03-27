#define WIN32_LEAN_AND_MEAN
#define NOGDI

#include <windows.h>
#include <stdio.h>
#include <stdatomic.h>

#include "sol.h"

// --- Shared state between threads ---
static atomic_bool g_running = TRUE;
static LARGE_INTEGER g_startTime, g_frequency;
static HWND g_hwnd = NULL;

// --- Forward declarations ---
static DWORD WINAPI GameThreadProc(LPVOID lpParam);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ─────────────────────────────────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    const char CLASS_NAME[] = "SolVulk";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(
        0, CLASS_NAME, "Sol Vulkan",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800,
        NULL, NULL, hInstance, NULL);
    if (!g_hwnd)
        return 1;

    ShowWindow(g_hwnd, nShowCmd);

    // Timing init on the main thread so Sol_Init can use g_frequency/g_startTime
    QueryPerformanceFrequency(&g_frequency);
    QueryPerformanceCounter(&g_startTime);

    //------------------------------------------
    // Init Sol App
    //------------------------------------------

    World *menu = World_Create();
    World_System_Add(menu, Sol_System_Interact_Ui, SYSTEM_TICK);
    World_System_Add(menu, Sol_System_Button_Update, SYSTEM_TICK);
    World_System_Add(menu, Sol_System_Info_Tick, SYSTEM_TICK);

    World_System_Add(menu, Sol_System_Step_Physx_2d, SYSTEM_STEP);
    World_System_Add(menu, Sol_System_Step_Physx_3d, SYSTEM_STEP);

    World_System_Add(menu, Sol_System_Update_View, SYSTEM_DRAW);

    int button = Sol_Prefab_Button(menu, (vec3s){10, 400, 0});
    Entity_Add_Body(menu, button, (CompBody){
                                      .vel = {0, -20.0f, 0},
                                      .height = menu->shapes[button].height,
                                      .width = menu->shapes[button].width,
                                  });
    Entity_Add_Info(menu, button, (CompInfo){.name = "BUTTON1"});

    int wizard = Sol_Prefab_Wizard(menu, (vec3s){20, 2, 0});
    // int wizard = Entity_Create(menu);
    // Entity_Add_Body(menu, wizard, (CompBody){.height = 5, .mass = 5, .vel = {0, 5, 0}});

    World *evansWorld = World_Create();
    evansWorld->worldActive = false;

    World *game = World_Create();

    World *worlds[] = {
        menu,
        game,
        evansWorld,
    };
    SolConfig solConfig = {
        .worlds = worlds,
        .worldCount = 2,
    };

    Sol_Init(g_hwnd, hInstance, solConfig);

    //------------------------------------------

    // Spin up the game loop on its own thread BEFORE entering the message pump
    HANDLE hGameThread = CreateThread(NULL, 0, GameThreadProc, NULL, 0, NULL);

    // Main thread is now 100% dedicated to pumping Windows messages.
    // It will never stall your game loop again.
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) // blocks until a message arrives – zero CPU waste
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Signal the game thread to stop and wait for it to finish cleanly
    g_running = FALSE;
    WaitForSingleObject(hGameThread, INFINITE);
    CloseHandle(hGameThread);

    return (int)msg.wParam;
}

// ─────────────────────────────────────────────────────────────────────────────
// Game loop – runs on its own thread, completely independent of Windows events
// ─────────────────────────────────────────────────────────────────────────────
static DWORD WINAPI GameThreadProc(LPVOID lpParam)
{
    LARGE_INTEGER lastTime, currentTime;
    QueryPerformanceCounter(&lastTime);

    while (g_running)
    {
        QueryPerformanceCounter(&currentTime);
        double dt = (double)(currentTime.QuadPart - lastTime.QuadPart) / (double)g_frequency.QuadPart;
        double runTime = (double)(currentTime.QuadPart - g_startTime.QuadPart) / (double)g_frequency.QuadPart;
        lastTime = currentTime;
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        Sol_Tick(dt, runTime);
    }

    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        g_running = FALSE;  // tell the game thread to exit
        PostQuitMessage(0); // tell the message loop to exit
        return 0;
    case WM_SIZE:
        Sol_Window_Resize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_KEYDOWN:
        SolInput_OnKey((int)wParam, true);
        return 0;
    case WM_KEYUP:
        SolInput_OnKey((int)wParam, false);
        return 0;
    case WM_MOUSEMOVE:
        SolInput_OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_LBUTTONDOWN:
        SolInput_OnMouseButton(SOL_MOUSE_LEFT, true);
        return 0;
    case WM_LBUTTONUP:
        SolInput_OnMouseButton(SOL_MOUSE_LEFT, false);
        return 0;
    case WM_RBUTTONDOWN:
        SolInput_OnMouseButton(SOL_MOUSE_RIGHT, true);
        return 0;
    case WM_RBUTTONUP:
        SolInput_OnMouseButton(SOL_MOUSE_RIGHT, false);
        return 0;
    case WM_MBUTTONDOWN:
        SolInput_OnMouseButton(SOL_MOUSE_MIDDLE, true);
        return 0;
    case WM_MBUTTONUP:
        SolInput_OnMouseButton(SOL_MOUSE_MIDDLE, false);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}