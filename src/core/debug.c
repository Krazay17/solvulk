#include "sol/sol.h"

#define MAX_DEBUGS 16
#define MAX_STR_LEN 64

typedef struct Debuggers
{
    int   characterCount[MAX_DEBUGS];
    char  label[MAX_DEBUGS][MAX_STR_LEN];
    float value[MAX_DEBUGS];
    char  textValue[MAX_DEBUGS][MAX_STR_LEN];
    int   count;
} Debuggers;

static Debuggers debuggers;
static double    fps;
static double    total, throttle;
static char      fpsbuffer[64];
static int       count;

void Sol_Debug_Add(const char *label, float value)
{
    for (int i = 0; i < debuggers.count; ++i)
    {
        if (strncmp(label, debuggers.label[i], MAX_STR_LEN) == 0)
        {
            debuggers.value[i] = value;
            return;
        }
    }

    if (debuggers.count > MAX_DEBUGS)
        return;

    strncpy(debuggers.label[debuggers.count], label, MAX_STR_LEN - 1);
    debuggers.label[debuggers.count][MAX_STR_LEN - 1] = '\0';
    debuggers.value[debuggers.count]                  = value;

    debuggers.count++;
}

void Sol_Debug_AddText(const char *label, const char *value)
{
    for (int i = 0; i < debuggers.count; ++i)
    {
        if (strncmp(label, debuggers.label[i], MAX_STR_LEN) == 0)
        {
            strncpy(debuggers.textValue[i], value, MAX_STR_LEN - 1);
            debuggers.textValue[i][MAX_STR_LEN - 1] = '\0';

            return;
        }
    }

    if (debuggers.count > MAX_DEBUGS)
        return;
    strncpy(debuggers.label[debuggers.count], label, MAX_STR_LEN - 1);
    debuggers.label[debuggers.count][MAX_STR_LEN - 1] = '\0';

    strncpy(debuggers.textValue[debuggers.count], value, MAX_STR_LEN - 1);
    debuggers.textValue[debuggers.count][MAX_STR_LEN - 1] = '\0';

    debuggers.count++;
}

void Sol_Debug_Draw(double dt)
{
    if (!solState.debug)
        return;

    float     offset  = 48.0f;
    float     spacing = 24.0f;
    RectSSBO *rect    = Sol_Render_GetNext_Rect(UILAYER_2);
    rect->rect        = (vec4s){ 0, 0, 200.0f, offset + spacing * debuggers.count };
    rect->color       = (vec4s){ 0.1f, 0.0f, 0.3f, 0.7f };
    rect->scale       = 1.0f;
    rect->fill        = 1.0f;
    rect->flags       = 0;
    for (int i = 0; i < debuggers.count; ++i)
    {
        char buffer[MAX_STR_LEN * 2];
        if (debuggers.textValue[i][0] != 0)
        {
            sprintf(buffer, "%s: %s", debuggers.label[i], debuggers.textValue[i]);
        }
        else
            sprintf(buffer, "%s: %.4f", debuggers.label[i], debuggers.value[i]);
        SolFontDesc fontDesc = {
            .layer = UILAYER_2,
            .x     = 6.0f,
            .y     = i * spacing + offset,
            .size  = 16.0f,
            .color = (vec4s){ 255, 0, 122, 255 },
            .kind  = SOL_FONT_ICE,
        };
        Sol_Render_DrawText(buffer, fontDesc);
    }

    SolFontDesc fontDesc = {
        .layer = UILAYER_2,
        .x     = 6.0f,
        .y     = 24.0f,
        .size  = 24.0f,
        .color = (vec4s){ 0, 1, 0, 1 },
        .kind  = SOL_FONT_ICE,
    };
    Sol_Render_DrawText(fpsbuffer, fontDesc);
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

// SolRayResult Sol_RaycastD(World *world, SolRay ray, float debugDuration)
// {
// SolRayResult result = Sol_Raycast(world, ray);
// if (solState.debug)
// {
//     Sol_Line_Push(world, (SolLine){
//                              .a      = ray.pos,
//                              .b      = result.pos,
//                              .aColor = (vec4s){1, 0, 0, 1},
//                              .bColor = (vec4s){1, 0, 0, 1},
//                              .ttl    = debugDuration,
//                          });
//     if (result.hit)
//         Sol_Line_Push(world, (SolLine){
//                                  .a = result.pos,
//                                  .b = glms_vec3_add(result.pos, glms_vec3_scale(ray.dir, ray.dist - result.dist)),
//                                  .aColor = (vec4s){0, 1, 0, 1},
//                                  .bColor = (vec4s){0, 1, 0, 1},
//                                  .ttl    = debugDuration,
//                              });
// }
// return result;
// }