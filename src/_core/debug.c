#include "sol_core.h"

static Debuggers debuggers = {0};

static void DebugFPS(double dt);

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
    debuggers.value[debuggers.count]                 = value;

    debuggers.count++;
}

void Sol_Debug_Draw(double dt)
{
    if (!Sol_GetState()->debug)
        return;
    float offset  = 48.0f;
    float spacing = 24.0f;
    Sol_Draw_Rectangle((SolRect){0, 0, 200.0f, offset + spacing * debuggers.count}, (vec4s){8, 0, 23, 222}, 0);
    for (int i = 0; i < debuggers.count; ++i)
    {
        char buffer[MAX_STR_LEN];
        sprintf(buffer, "%s: %.4f", debuggers.text[i], debuggers.value[i]);
        Sol_Draw_Text(buffer, 6.0f, i * spacing + offset, 16.0f, (vec4s){255, 0, 122, 255}, SOL_FONT_ICE);
    }
    DebugFPS(dt);

    // static int limiter;
    // if(limiter++ % 100 == 0)
    // printf("rand %d\n", rand()%255);
}

static void DebugFPS(double dt)
{
    SolState     *state = Sol_GetState();
    static double total, throttle;
    static char   buffer[64];
    static int    count;
    if (dt < FLOATING_EPSILON)
        return;
    state->fps = 1.0 / dt;
    total += state->fps;
    count++;

    if ((throttle += dt) > 0.1)
    {
        float currentFps = total / count;
        snprintf(buffer, sizeof(buffer), "Fps: %.0f", currentFps);
        throttle = 0;
        count    = 0;
        total    = 0;
    }
    Sol_Draw_Text(buffer, 6.0f, 24.0f, 24.0f, (vec4s){0, 255, 0, 255}, SOL_FONT_ICE);
}