#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
// #define NOGDI
#include <dwmapi.h>
#include <windows.h>
#pragma comment(lib, "dwmapi.lib")
#define IS_WINDOWS 1
#else
#define IS_WINDOWS 0
#endif

#include "game.h"

#define TARGET_FRAME_TIME (1.0 / 500.0)

// --- Shared state between threads ---
static volatile g_running = TRUE;
static HWND g_hwnd        = NULL;

static bool  isDragging = false;
static POINT dragStartPos;

HMODULE current_engine_lib = NULL;

// --- Forward declarations ---
static DWORD WINAPI GameThreadProc(LPVOID lpParam);
LRESULT CALLBACK    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// HOT REALOAD
#define SOL_FUNC(ret, name, ...)                                                                                       \
    typedef ret (*name##_fn)(__VA_ARGS__);                                                                             \
    name##_fn pfn_##name;
#include "sol/functions.inc"
#undef SOL_FUNC

void     load_api(const char *path);
FILETIME get_last_write_time(const char *path);

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    void *platform_handle = NULL;

#ifdef IS_WINDOWS
    HINSTANCE hInstance = GetModuleHandle(NULL);
    int       nShowCmd  = SW_SHOWDEFAULT;
#endif

    const char CLASS_NAME[] = "SolVulk";
    WNDCLASS   wc           = {0};
    wc.lpfnWndProc          = WindowProc;
    wc.hInstance            = hInstance;
    wc.lpszClassName        = CLASS_NAME;
    wc.hCursor              = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(WS_EX_TOPMOST, CLASS_NAME, "Sol Vulkan",
                            WS_POPUP | WS_VISIBLE, 720, 0, 1200, 800, NULL, NULL, hInstance, NULL);
    if (!g_hwnd)
        return 1;

    MARGINS margins = {-1}; // -1 extends to the entire window
    DwmExtendFrameIntoClientArea(g_hwnd, &margins);

    // CLICKTHROUGH
    // LONG_PTR exStyle = GetWindowLongPtr(g_hwnd, GWL_EXSTYLE);
    // exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
    // SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, exStyle);

    ShowWindow(g_hwnd, nShowCmd);

    // AllocConsole();
    // freopen("CONOUT$", "w", stdout);

    // Raw input device for accurate mouse
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x02;
    rid.dwFlags     = 0;
    rid.hwndTarget  = g_hwnd;
    if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == false)
    {
        printf("Raw input register failed!");
    }

    //------------------------------------------
    // Init Sol App
    //------------------------------------------
    Sol_Init(g_hwnd, hInstance);
    // load hot reload api
    // load_api("libsolvulk.dll");

    Create_Sol_Game();

    //------------------------------------------

    // Spin up the game loop on its own thread BEFORE entering the message pump
    HANDLE hGameThread = CreateThread(NULL, 0, GameThreadProc, NULL, 0, NULL);

    // Main thread is now 100% dedicated to pumping Windows messages.
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) // blocks until a message arrives – zero CPU waste
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    InterlockedExchange(&g_running, 0);
    WaitForSingleObject(hGameThread, INFINITE);
    CloseHandle(hGameThread);

    return (int)msg.wParam;
}

// ─────────────────────────────────────────────────────────────────────────────
// Game loop – runs on its own thread, completely independent of Windows events
// ─────────────────────────────────────────────────────────────────────────────
static DWORD WINAPI GameThreadProc(LPVOID lpParam)
{
    LARGE_INTEGER startTime, lastTime, currentTime, endTime, freq;
    QueryPerformanceCounter(&startTime);
    lastTime = currentTime = startTime;

    while (InterlockedAdd(&g_running, 0))
    {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&currentTime);
        double dt      = (double)(currentTime.QuadPart - lastTime.QuadPart) / (double)freq.QuadPart;
        double runTime = (double)(currentTime.QuadPart - startTime.QuadPart) / (double)freq.QuadPart;
        lastTime       = currentTime;
        POINT cursorPos;

        GetCursorPos(&cursorPos);
        Sol_Tick(dt, runTime);

        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&endTime);
        double elapsed   = (double)(endTime.QuadPart - currentTime.QuadPart) / (double)freq.QuadPart;
        double remaining = TARGET_FRAME_TIME - elapsed;
        if (remaining > 0.001)
            Sleep((DWORD)(remaining * 1000.0));
        while (1)
        {
            QueryPerformanceCounter(&endTime);
            double e = (double)(endTime.QuadPart - currentTime.QuadPart) / (double)freq.QuadPart;
            if (e >= TARGET_FRAME_TIME)
                break;
        }
    }

    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        InterlockedExchange(&g_running, 0);
        PostQuitMessage(0);
        return 0;

    case WM_INPUT: {
        UINT        dwSize = sizeof(RAWINPUT);
        static BYTE lpb[sizeof(RAWINPUT)]; // Static buffer for performance

        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

        RAWINPUT *raw = (RAWINPUT *)lpb;

        if (raw->header.dwType == RIM_TYPEMOUSE)
        {
            LONG mouseX = raw->data.mouse.lLastX;
            LONG mouseY = raw->data.mouse.lLastY;
            if (!(raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
            {
                Sol_Input_OnRawMouse(mouseX, mouseY);
            }
        }
        return 0;
    }
    // case WM_NCHITTEST: {
    //     POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
    //     ScreenToClient(hwnd, &pt);
    //     if (pt.y < 30)
    //         return HTCAPTION; // drag bar
    //     // if (IsOverButton(pt))
    //     //     return HTCLIENT;  // interactive
    //     //return HTTRANSPARENT; // everything else: click-through
    //     return 0;
    // }
    case WM_SIZE:
        Sol_Window_Resize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_PAINT:
        ValidateRect(hwnd, NULL);
        return 0;
    case WM_SETCURSOR:
        SolMouse mouse = SolInput_GetMouse();
        if (mouse.locked)
        {
            SetCursor(NULL);
            return true;
        }
        break;
    case WM_KEYDOWN:
        SolInput_OnKey((int)wParam, true);
        return 0;
    case WM_KEYUP:
        SolInput_OnKey((int)wParam, false);
        return 0;
    case WM_MOUSEMOVE:
        if (isDragging)
        {
            POINT currentPos;
            GetCursorPos(&currentPos);
            RECT windowRect;
            GetWindowRect(hwnd, &windowRect);
            int newX = windowRect.left + (currentPos.x - dragStartPos.x);
            int newY = windowRect.top + (currentPos.y - dragStartPos.y);

            SetWindowPos(hwnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            dragStartPos = currentPos;
        }

        SolInput_OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_LBUTTONDOWN:
        SolInput_OnMouseButton(SOL_MOUSE_LEFT, true);
        POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
        if (pt.y < 30)
        {
            isDragging = true;
            SetCapture(hwnd); // Ensures you get mouse input even if the mouse leaves the window
            GetCursorPos(&dragStartPos);
        }
        return 0;
    case WM_LBUTTONUP:
        SolInput_OnMouseButton(SOL_MOUSE_LEFT, false);
        if (isDragging)
        {
            isDragging = false;
            ReleaseCapture();
        }
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

void QuitApp(void *data)
{
    PostMessage(g_hwnd, WM_DESTROY, 0, 0);
}

// HOT RELOAD LOGIC
void load_api(const char *path)
{
    // 1. If we already have a lib open, free it
    if (current_engine_lib)
        FreeLibrary(current_engine_lib);

    // 2. Copy the DLL so the original isn't locked by the OS
    // FALSE means "allow overwrite"
    CopyFile(path, "solvulk_live.dll", FALSE);

    // 3. Load the COPY
    current_engine_lib = LoadLibraryA("solvulk_live.dll");
    if (!current_engine_lib)
    {
        printf("FAILED TO LOAD ENGINE DLL!\n");
        return;
    }

// 4. Map the pointers
#define SOL_FUNC(ret, name, ...) pfn_##name = (name##_fn)GetProcAddress(current_engine_lib, #name);
#include "sol/functions.inc"
#undef SOL_FUNC
}

FILETIME get_last_write_time(const char *path)
{
    FILETIME                  lastWriteTime = {0};
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }
    return lastWriteTime;
}
