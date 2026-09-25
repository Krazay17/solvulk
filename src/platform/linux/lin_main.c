/*
 * File: lin_main.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-25
 * 
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "game.h"
#include "sol_core.h"
#include "platform/platform.h"

#define TARGET_FRAME_TIME (1.0 / 600.0)

// --- Shared state between threads ---
static atomic_int g_running = 1;
static Display *g_display = NULL;
static Window g_window = 0;
static Atom g_wmDeleteMessage;
static bool isFullscreen = false;

static bool isDragging = false;
static int dragStartMouseX, dragStartMouseY;
static int dragStartWinX, dragStartWinY;

// --- Forward declarations ---
static void *GameThreadProc(void *arg);
static void ProcessXEvent(XEvent *ev);
static double GetTimeInSeconds(void);

// Motif hints struct for borderless / undecorated window creation (WS_POPUP equivalent)
typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} Hints;

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    XInitThreads();

    g_display = XOpenDisplay(NULL);
    if (!g_display)
    {
        fprintf(stderr, "[SolVulk Fatal Error] Failed to open X11 Display!\n");
        return 1;
    }

    int screen = DefaultScreen(g_display);
    Window root = RootWindow(g_display, screen);

    XSetWindowAttributes attr = {0};
    attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                      ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                      StructureNotifyMask | FocusChangeMask;

    g_window = XCreateWindow(
        g_display, root,
        640, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
        CopyFromParent, InputOutput, CopyFromParent,
        CWEventMask, &attr
    );

    if (!g_window)
    {
        fprintf(stderr, "[SolVulk Fatal Error] Window Creation Failed!\n");
        XCloseDisplay(g_display);
        return 1;
    }

    // Borderless / Popup Window Style (MWM_HINTS_DECORATIONS = 2)
    Hints hints = { .flags = 2, .decorations = 0 };
    Atom motifProperty = XInternAtom(g_display, "_MOTIF_WM_HINTS", True);
    if (motifProperty != None)
    {
        XChangeProperty(g_display, g_window, motifProperty, motifProperty, 32, PropModeReplace, (unsigned char *)&hints, 5);
    }

    // Set Window Title
    XStoreName(g_display, g_window, "Solblade");

    // Catch WM_DELETE_WINDOW message from window manager
    g_wmDeleteMessage = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_display, g_window, &g_wmDeleteMessage, 1);

    XMapWindow(g_display, g_window);
    XFlush(g_display);

    // ------------------------------------------
    // Init Sol App (Pass Display* and Window handle)
    // ------------------------------------------
    if (Sol_Init((void *)g_display, (void *)(uintptr_t)g_window) != 0)
    {
        fprintf(stderr, "[SolVulk Fatal Error] Vulkan Initialization Failed!\n");
        XDestroyWindow(g_display, g_window);
        XCloseDisplay(g_display);
        return 1;
    }

    if (!solState.isRunning)
    {
        fprintf(stderr, "[Error] Engine flagged as not running before event loop started.\n");
        return 1;
    }

    Create_Sol_Game();

    // Spin up the game loop on its own thread
    pthread_t gameThread;
    if (pthread_create(&gameThread, NULL, GameThreadProc, NULL) != 0)
    {
        fprintf(stderr, "[Fatal Error] Failed to create Game Thread!\n");
        return 1;
    }

    // Main thread pumps X11 events
    while (atomic_load(&g_running) && solState.isRunning)
    {
        while (XPending(g_display) > 0)
        {
            XEvent ev;
            XNextEvent(g_display, &ev);
            ProcessXEvent(&ev);
        }
        usleep(1000); // Prevent 100% CPU lock on event thread
    }

    atomic_store(&g_running, 0);
    pthread_join(gameThread, NULL);

    XDestroyWindow(g_display, g_window);
    XCloseDisplay(g_display);

    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// High-precision clock helper
// ─────────────────────────────────────────────────────────────────────────────
static double GetTimeInSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// ─────────────────────────────────────────────────────────────────────────────
// Game loop – runs on dedicated thread independent of X11 events
// ─────────────────────────────────────────────────────────────────────────────
static void *GameThreadProc(void *arg)
{
    double startTime = GetTimeInSeconds();
    double lastTime = startTime;

    while (atomic_load(&g_running))
    {
        double currentTime = GetTimeInSeconds();
        double dt = currentTime - lastTime;
        double runTime = currentTime - startTime;
        lastTime = currentTime;

        Sol_Tick(dt, runTime);

        double endTime = GetTimeInSeconds();
        double elapsed = endTime - currentTime;
        double remaining = TARGET_FRAME_TIME - elapsed;

        if (remaining > 0.001)
        {
            struct timespec req = {
                .tv_sec = (time_t)remaining,
                .tv_nsec = (long)((remaining - (time_t)remaining) * 1e9)
            };
            nanosleep(&req, NULL);
        }

        while (1)
        {
            double e = GetTimeInSeconds() - currentTime;
            if (e >= TARGET_FRAME_TIME)
                break;
        }
    }

    return NULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// X11 Event Handler
// ─────────────────────────────────────────────────────────────────────────────
static void ProcessXEvent(XEvent *ev)
{
    switch (ev->type)
    {
    case FocusOut:
        Sol_Input_SetLocked(false);
        Sol_Input_Clear();
        break;

    case ClientMessage:
        if ((Atom)ev->xclient.data.l[0] == g_wmDeleteMessage)
        {
            solState.destroy_qued = true;
            atomic_store(&g_running, 0);
        }
        break;

    case KeyPress:
    case KeyRelease: {
        bool isDown = (ev->type == KeyPress);
        KeySym keysym = XLookupKeysym(&ev->xkey, 0);
        Sol_Input_OnKey((int)keysym, isDown);
        break;
    }

    case MotionNotify: {
        int x = ev->xmotion.x;
        int y = ev->xmotion.y;

        if (isDragging)
        {
            int rootX = ev->xmotion.x_root;
            int rootY = ev->xmotion.y_root;
            int newWinX = dragStartWinX + (rootX - dragStartMouseX);
            int newWinY = dragStartWinY + (rootY - dragStartMouseY);
            XMoveWindow(g_display, g_window, newWinX, newWinY);
        }

        static int lastX = 0, lastY = 0;
        int dx = x - lastX;
        int dy = y - lastY;
        lastX = x;
        lastY = y;

        Sol_Input_OnRawMouse(dx, dy);
        Sol_Input_OnMouseMove(x, y);
        break;
    }

    case ButtonPress:
    case ButtonRelease: {
        bool isDown = (ev->type == ButtonPress);
        unsigned int button = ev->xbutton.button;

        if (button == Button1) // Left Click
        {
            Sol_Input_OnMouseButton(SOL_MOUSE_LEFT, isDown);
            if (!isFullscreen && isDown && ev->xbutton.y < 30)
            {
                isDragging = true;
                dragStartMouseX = ev->xbutton.x_root;
                dragStartMouseY = ev->xbutton.y_root;

                Window child;
                XTranslateCoordinates(g_display, g_window, RootWindow(g_display, DefaultScreen(g_display)),
                                     0, 0, &dragStartWinX, &dragStartWinY, &child);
            }
            else if (!isDown && isDragging)
            {
                isDragging = false;
            }
        }
        else if (button == Button2) // Middle Click
        {
            Sol_Input_OnMouseButton(SOL_MOUSE_MIDDLE, isDown);
        }
        else if (button == Button3) // Right Click
        {
            Sol_Input_OnMouseButton(SOL_MOUSE_RIGHT, isDown);
        }
        else if (button == Button4 && isDown) // Mouse Wheel Up
        {
            Sol_Input_OnMouseWheel(120);
        }
        else if (button == Button5 && isDown) // Mouse Wheel Down
        {
            Sol_Input_OnMouseWheel(-120);
        }
        break;
    }

    case ConfigureNotify:
        Sol_Window_OnResize(ev->xconfigure.x, ev->xconfigure.y,
                            ev->xconfigure.width, ev->xconfigure.height);
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Window Commands
// ─────────────────────────────────────────────────────────────────────────────
void Sol_Quit(void)
{
    solState.destroy_qued = true;
    atomic_store(&g_running, 0);
}

void QuitApp(int flags)
{
    Sol_Quit();
}

void W_Set_Ontop(int flags)
{
    bool toggle = (flags & INTERACT_TOGGLED);
    Atom wmState = XInternAtom(g_display, "_NET_WM_STATE", False);
    Atom wmAbove = XInternAtom(g_display, "_NET_WM_STATE_ABOVE", False);

    XEvent xev = {0};
    xev.type = ClientMessage;
    xev.xclient.window = g_window;
    xev.xclient.message_type = wmState;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = toggle ? 1 : 0; // 1 = _NET_WM_STATE_ADD, 0 = _NET_WM_STATE_REMOVE
    xev.xclient.data.l[1] = wmAbove;

    XSendEvent(g_display, DefaultRootWindow(g_display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

void W_Set_Fullscreen(int flags)
{
    isFullscreen = (bool)flags;
    Atom wmState = XInternAtom(g_display, "_NET_WM_STATE", False);
    Atom wmFullscreen = XInternAtom(g_display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent xev = {0};
    xev.type = ClientMessage;
    xev.xclient.window = g_window;
    xev.xclient.message_type = wmState;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = isFullscreen ? 1 : 0;
    xev.xclient.data.l[1] = wmFullscreen;

    XSendEvent(g_display, DefaultRootWindow(g_display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

// ─────────────────────────────────────────────────────────────────────────────
// Platform File Resource API
// ─────────────────────────────────────────────────────────────────────────────
void Sol_Free_Resource(SolResource *res)
{
    if (res->isHeap && res->data)
    {
        free(res->data);
        res->data = NULL;
    }
}

int Sol_ReadFile(const char *filename, SolResource *outRes)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
        return 0;

    fseek(file, 0, SEEK_END);
    outRes->size = ftell(file);
    rewind(file);

    outRes->data = malloc(outRes->size);
    if (!outRes->data)
    {
        fclose(file);
        return 0;
    }

    fread(outRes->data, 1, outRes->size, file);
    fclose(file);
    outRes->isHeap = 1;
    return 1;
}

int Sol_WriteFile(const char *prefix, const char *filename, const void *buffer, const size_t size)
{
    char disk_path[128];
    snprintf(disk_path, sizeof(disk_path), "assets/%s%s", prefix, filename);
    FILE *file = fopen(disk_path, "wb");
    if (!file)
        return 0;

    size_t written = fwrite(buffer, 1, size, file);
    fclose(file);
    return written == size;
}

int Sol_DeleteFile(const char *prefix, const char *filename)
{
    char disk_path[128];
    snprintf(disk_path, sizeof(disk_path), "assets/%s%s", prefix, filename);
    return remove(disk_path) == 0;
}

SolResource Sol_LoadResource(const char *resourceName, const char *prefix)
{
    SolResource res = {0};
    char disk_path[128];
    snprintf(disk_path, sizeof(disk_path), "assets/%s%s", prefix, resourceName);

    if (Sol_ReadFile(disk_path, &res))
    {
        return res;
    }

    printf("Failed to find resource %s at %s\n", resourceName, disk_path);
    return res;
}