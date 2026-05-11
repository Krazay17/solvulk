#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "sol_core.h"


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