#include "sol_core.h"

static DebugLines lines = {0};

void Sol_Debug_Add(const char *text, float value)
{
    printf("DEBUG ADD\n");
    for(int i = 0; i < lines.count;++i)
    {
        if(strncmp(text, lines.text[i], MAX_STR_LEN) == 0)
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
    float spacing = 24.0f;
    for (int i = 0; i < lines.count; ++i)
    {
        const char *buffer;
        //sprintf(buffer, )
        Sol_Draw_Text(lines.text[i], 24.0f, i * spacing + 48.0f, 16.0f, (SolColor){255,0,122,255});
    }
}