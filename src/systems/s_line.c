#include "sol_core.h"

#define MAX_WORLD_LINES 0xffffff

typedef struct WorldLines
{
    SolLine lines[MAX_WORLD_LINES];
    int     count;
} WorldLines;

void Sol_Line_Init(World *world)
{
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

void Sol_Line_Tick(World *world, double dt, double time)
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

void Sol_Line_Draw(World *world, double dt, double time)
{
    WorldLines *lines = world->lines;
    if (lines->count == 0)
        return;
    Render_Draw_Line(lines->lines, lines->count);
}