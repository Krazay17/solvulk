#include "sol/sol.h"
#include "sol_engine.h"


#define MAX_DEBUGS 14
#define MAX_STR_LEN 64

typedef struct Debuggers
{
    int   characterCount[MAX_DEBUGS];
    char  text[MAX_DEBUGS][MAX_STR_LEN];
    float value[MAX_DEBUGS];
    int   count;
} Debuggers;

static Debuggers debuggers;
static double    fps;
static double    total, throttle;
static char      fpsbuffer[64];
static int       count;

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
    if (!solState.debug)
        return;
    float offset  = 48.0f;
    float spacing = 24.0f;
    Sol_Render_DrawRectangle((vec4s){0, 0, 200.0f, offset + spacing * debuggers.count}, (vec4s){0.1f, 0.0f, 0.3f, 0.7f}, 0, 1.0f);
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
        Sol_Render_DrawText(fontDesc);
    }

    SolFontDesc fontDesc = {
        .str   = fpsbuffer,
        .x     = 6.0f,
        .y     = 24.0f,
        .size  = 24.0f,
        .color = (vec4s){0, 1, 0, 1},
        .kind  = SOL_FONT_ICE,
    };
    Sol_Render_DrawText(fontDesc);
}

void Sol_FPS(double dt)
{
    if (dt < FLOATING_EPSILON)
        return;
    solState.fps = 1.0 / dt;
    total += solState.fps;
    count++;

    if ((throttle += dt) > 0.1)
    {
        float currentFps = total / count;
        snprintf(fpsbuffer, sizeof(fpsbuffer), "Fps: %.0f", currentFps);
        throttle = 0;
        count    = 0;
        total    = 0;
    }
}

SolRayResult Sol_RaycastD(World *world, SolRay ray, float debugDuration)
{
    SolRayResult result = Sol_Raycast(world, ray);
    Sol_Line_Add(world, (LineDesc){
                            .a      = ray.pos,
                            .b      = result.pos,
                            .colorA = (vec4s){1, 0, 0, 1},
                            .colorB = (vec4s){1, 0, 0, 1},
                            .ttl    = debugDuration,
                        });
    if (result.hit)
        Sol_Line_Add(world, (LineDesc){
                                .a      = result.pos,
                                .b      = glms_vec3_add(result.pos, glms_vec3_scale(ray.dir, ray.dist - result.dist)),
                                .colorA = (vec4s){0, 1, 0, 1},
                                .colorB = (vec4s){0, 1, 0, 1},
                                .ttl    = debugDuration,
                            });
    return result;
}