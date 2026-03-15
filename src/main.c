#include <windows.h>
#include <stdio.h>

static LARGE_INTEGER frequency, lastTime, currentTime, startTime, runTime;

static void tick();
void Sol_Init(HWND hwnd, HINSTANCE hInstance);
void Sol_Tick(double dt, double time);
double GetRunTime();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    const char CLASS_NAME[] = "HelloTriangle";

    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "Hello Triangle",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);
    if (!hwnd)
        return 1;
    ShowWindow(hwnd, nShowCmd);
    Sol_Init(hwnd, hInstance);

    QueryPerformanceCounter(&startTime);
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);
    
    MSG msg = {0};
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        tick();
    }

    return 0;
}

static void tick()
{
    QueryPerformanceCounter(&currentTime);
    double dt = (double)(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
    lastTime = currentTime;
    Sol_Tick(dt, GetRunTime());
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ENTERSIZEMOVE:
        // User started dragging or resizing.
        // Set a timer to fire as fast as possible (0ms delay).
        SetTimer(hwnd, 1, USER_TIMER_MINIMUM, NULL);
        return 0;

    case WM_EXITSIZEMOVE:
        // User let go. Kill the timer so we go back to the main loop.
        KillTimer(hwnd, 1);
        return 0;

    case WM_TIMER:
        // This fires even while Windows is "frozen" in the move loop.
        if (wParam == 1)
        {
            tick();
        }
        return 0;

    case WM_PAINT:
        // Ensure the window redraws if it gets uncovered
        tick();
        ValidateRect(hwnd, NULL);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

double GetRunTime()
{
    return (double)(currentTime.QuadPart - startTime.QuadPart) / 100000;
}