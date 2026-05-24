#include "sol_core.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void Sol_MessageBox(const char *text, const char *level)
{
    MessageBox(NULL, text, level, MB_OK);
}