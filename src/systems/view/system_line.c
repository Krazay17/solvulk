#include "sol_core.h"

void Lines_Init(World *world)
{
    world->lines = calloc(1, sizeof(WorldLines));
}

void Sol_World_Line_Add(World *world, vec3s a, vec3s b, vec3s color, vec3s bColor, float ttl)
{
    WorldLines *lines = world->lines;
    if (lines->count >= MAX_WORLD_LINES)
        return;
    lines->lines[lines->count++] = (SolLine){a, b, color, bColor, ttl};
}

void Sol_System_Line_Tick(World *world, double dt, double time)
{
    WorldLines *lines = world->lines;
    int write = 0;
    for (int i = 0; i < lines->count; i++)
    {
        lines->lines[i].ttl -= (float)dt;
        if (lines->lines[i].ttl >= 0)
            lines->lines[write++] = lines->lines[i];
    }
    lines->count = write;
}

void Sol_System_Line_Draw(World *world, double dt, double time)
{
    WorldLines *lines = world->lines;
    if (lines->count == 0)
        return;
    Sol_Draw_Line(lines->lines, lines->count);
}