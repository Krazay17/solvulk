#include <windows.h>
#include <stdio.h>
#include <stdatomic.h>

#include "input.h"

// --- Shared state between threads ---
static atomic_bool g_running = TRUE;
static atomic_bool g_needsResize = FALSE;

static HWND g_hwnd = NULL;

static LARGE_INTEGER g_startTime, g_frequency;

// --- Forward declarations ---
static DWORD WINAPI GameThreadProc(LPVOID lpParam);
void Sol_Init(HWND hwnd, HINSTANCE hInstance);
void Sol_Tick(double dt, double time);
void Sol_On_Resize();

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
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);
    if (!g_hwnd)
        return 1;

    ShowWindow(g_hwnd, nShowCmd);

    // Timing init on the main thread so Sol_Init can use g_frequency/g_startTime
    QueryPerformanceFrequency(&g_frequency);
    QueryPerformanceCounter(&g_startTime);

    Sol_Init(g_hwnd, hInstance);

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
        if (g_needsResize)
        {
            g_needsResize = FALSE;
            Sol_On_Resize();
        }

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
        if (wParam != SIZE_MINIMIZED)
            g_needsResize = TRUE;
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