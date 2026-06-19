/*
 * File: s_line.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Lines!
 */
#include "s_line.h"
#include "world.h"

#define MAX_WORLD_LINES 0xffffff

typedef struct WorldLines
{
    SolLine lines[MAX_WORLD_LINES];
    int     count;
} WorldLines;

static void Line_Tick(World *world, double dt, double time);
static void Line_Draw(World *world, double dt, double time);

void Sol_Line_Init(World *world)
{
    WAddTick(world) = Line_Tick;
    WAdd3d(world)   = Line_Draw;

    world->lines = calloc(1, sizeof(WorldLines));
}

void Sol_Line_Add(World *world, LineDesc desc)
{
    if (!world || !world->lines)
        return;
    WorldLines *lines = world->lines;
    if (lines->count >= MAX_WORLD_LINES)
        return;
    lines->lines[lines->count++] = (SolLine){desc.a, desc.b, desc.colorA, desc.colorB, desc.ttl};
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