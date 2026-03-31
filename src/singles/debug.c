#include "sol_core.h"

static DebugLines lines = {0};

void Sol_Debug_Add(const char *text, float value)
{
    for (int i = 0; i < lines.count; ++i)
    {
        if (strncmp(text, lines.text[i], MAX_STR_LEN) == 0)
        {
            lines.value[i] = value;
            return;
        }
    }

    if (lines.count > MAX_DEBUGS)
        return;

    strncpy(lines.text[lines.count], text, MAX_STR_LEN - 1);
    lines.text[lines.count][MAX_STR_LEN - 1] = '\0';
    lines.value[lines.count] = value;

    lines.count++;
}

void Sol_Debug_Draw()
{
    float offset = 48.0f;
    float spacing = 24.0f;
    Sol_Draw_Rectangle((SolRect){0, 0, 200.0f, offset + spacing * lines.count}, (SolColor){8, 0, 23, 222}, 0);
    for (int i = 0; i < lines.count; ++i)
    {
        char buffer[MAX_STR_LEN];
        sprintf(buffer, "%s: %.4f", lines.text[i], lines.value[i]);
        Sol_Draw_Text(buffer, 6.0f, i * spacing + offset, 16.0f, (SolColor){255, 0, 122, 255});
    }
}