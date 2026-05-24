#include "sol_core.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static bool isLocked;
static POINT lockPos;

void Sol_Platform_LockCursor(bool lock)
{
    static POINT anchor;
    if (lock)
    {
        if (!isLocked)
        {
            GetCursorPos(&anchor);
            isLocked = true;
            ShowCursor(FALSE);
        }
        SetCursorPos(anchor.x, anchor.y);
    }
    else if (isLocked)
    {
        isLocked = false;
        ShowCursor(TRUE);
    }
}

void Sol_Platform_SetCursorpos(int x, int y)
{
    SetCursorPos(x, y);
}