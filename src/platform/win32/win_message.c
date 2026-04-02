#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "sol_core.h"

void Sol_MessageBox(const char *text, const char *level)
{
    MessageBox(NULL, text, level, MB_OK);
}