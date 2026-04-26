#include "sol_core.h"

static Debuggers debuggers = {0};

void Sol_Debug_Add(const char *text, float value)
{
    for (int i = 0; i < debuggers.count; ++i)
    {
        if (strncmp(text, debuggers.text[i], MAX_STR_LEN) == 0)
        {
            debuggers.value[i] = value;
            return;
        }
    }

    if (debuggers.count > MAX_DEBUGS)
        return;

    strncpy(debuggers.text[debuggers.count], text, MAX_STR_LEN - 1);
    debuggers.text[debuggers.count][MAX_STR_LEN - 1] = '\0';
    debuggers.value[debuggers.count] = value;

    debuggers.count++;
}

void Sol_Debug_Draw()
{
    float offset = 48.0f;
    float spacing = 24.0f;
    Sol_Draw_Rectangle((SolRect){0, 0, 200.0f, offset + spacing * debuggers.count}, (SolColor){8, 0, 23, 222}, 0);
    for (int i = 0; i < debuggers.count; ++i)
    {
        char buffer[MAX_STR_LEN];
        sprintf(buffer, "%s: %.4f", debuggers.text[i], debuggers.value[i]);
        Sol_Draw_Text(buffer, 6.0f, i * spacing + offset, 16.0f, (SolColor){255, 0, 122, 255});
    }
}