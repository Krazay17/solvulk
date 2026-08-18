/*
 * File: s_line.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Lines!
 */
#include "s_line.h"
#include "render/render.h"
#include "world.h"

#define MAX_WORLD_LINES 0xffff

typedef struct WorldLines
{
    SolLine lines[MAX_WORLD_LINES];
    int     count;
} WorldLines;

static void Line_Tick(World *world, double dt, double time);
static void Line_Draw(World *world, double dt, double time);

void Sol_Line_Init(World *world)
{
    world->lines        = malloc(sizeof(WorldLines));
    world->lines->count = 0;

    WAddTick(world) = Line_Tick;
    WAdd3d(world)   = Line_Draw;
}

SolLine *Sol_Line_New(World *w)
{
    if (!w || !w->lines)
        return NULL;

    WorldLines *lines = w->lines;
    if (lines->count >= MAX_WORLD_LINES)
        return NULL;

    return &lines->lines[lines->count++];
}

void Sol_Line_Push(World *world, SolLine desc)
{
    SolLine *line = Sol_Line_New(world);
    if (line)
        *line = desc;
}

static void Line_Tick(World *world, double dt, double time)
{
    WorldLines *lines = world->lines;
    int         write = 0;
    for (int i = 0; i < lines->count; i++)
    {
        lines->lines[i].ttl -= (float)dt;
        if (lines->lines[i].ttl >= 0)
        {
            lines->lines[write++] = lines->lines[i];
        }
    }
    lines->count = write;
}

static void Line_Draw(World *world, double dt, double time)
{
    WorldLines *lines = world->lines;
    if (lines->count == 0)
        return;
    Sol_Render_DrawLine(lines->lines, lines->count);
}