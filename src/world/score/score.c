#include "score.h"
#include "world.h"
#include "render/render.h"
#include "event/s_event.h"

typedef struct Score
{
    int   kills, deaths, assists;
    float damageDealt, healingDealt;
    float damageTaken;
} Score;

typedef struct SolScore
{
    Score score[MAX_ENTS];
    u8    tracked[MAX_ENTS];
} SolScore;

static void Score_Step(World *world, double dt, double time);
static void Score_Draw(World *world, double dt, double time);

void Sol_Score_Init(World *world)
{
    world->scores             = calloc(1, sizeof(SolScore));
    world->scores->tracked[1] = 1;
    WAddStep(world)           = Score_Step;
    WAdd2d(world)             = Score_Draw;
}

static void Score_Step(World *world, double dt, double time)
{
    SolScore  *scores = world->scores;
    SolEvents *events = world->events;
    for (int i = 0; i < events->count; i++)
    {
        SolEvent *event = &events->event[i];
        if (event->kind != EVENTKIND_SCORE)
            continue;
        if (event->as.score.damageDealt)
        {
            scores->score[event->entA].damageDealt += event->as.score.damageDealt;
        }
    }
}

static void Score_Draw(World *world, double dt, double time)
{
    SolScore *scores = world->scores;
    char      buffer[32];
    for (int i = 0; i < MAX_ENTS; i++)
    {
        if (!scores->tracked[i])
            continue;
        sprintf(buffer, "%.0f", scores->score[i].damageDealt);
        SolFontDesc font = {
            .color = {1.0f, 1.0f, 1.0f, 1.0f},
            .size  = 32.0f,
            .x     = 400.0f,
            .y     = 400.0f,
            .str   = buffer,
        };
        Sol_Render_DrawText2D(font);
        RectSSBO *rect = Sol_Render_GetNext_Rect();
        rect->dims     = (vec4s){100.0f, 50.0f, 1.0f, 1.0f};
        rect->pos      = (vec4s){400.0f, 400.0f, 0, 0};
    }
}