#include "sol_core.h"

#define MAX_DEBUGS 14
#define MAX_STR_LEN 64

typedef struct Debuggers
{
    int characterCount[MAX_DEBUGS];
    char text[MAX_DEBUGS][MAX_STR_LEN];
    float value[MAX_DEBUGS];
    int count;
} Debuggers;

static Debuggers debuggers = {0};

static void DebugFPS(double dt);
void Sol_Debug_Draw(double dt);

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
    Render_Draw_Rectangle((vec4s){0, 0, 200.0f, offset + spacing * debuggers.count}, (vec4s){8, 0, 23, 222}, 0);
    for (int i = 0; i < debuggers.count; ++i)
    {
        char buffer[MAX_STR_LEN];
        sprintf(buffer, "%s: %.4f", debuggers.text[i], debuggers.value[i]);
        SolFontDesc fontDesc = {
            .str   = buffer,
            .x     = 6.0f,
            .y     = i * spacing + offset,
            .size  = 16.0f,
            .color = (vec4s){255, 0, 122, 255},
            .kind  = SOL_FONT_ICE,
        };
        Sol_Draw_Text(fontDesc);
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
    SolFontDesc fontDesc = {
        .str   = buffer,
        .x     = 6.0f,
        .y     = 24.0f,
        .size  = 24.0f,
        .color = (vec4s){0, 255, 0, 255},
        .kind  = SOL_FONT_ICE,
    };
    Sol_Draw_Text(fontDesc);
}