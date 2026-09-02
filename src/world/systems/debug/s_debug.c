/*
 * File: s_line.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Lines!
 */

#include "world.h"
#include "render/render.h"

typedef struct WorldLines
{
    SolLine *lines;
} SysDebug;

void Debug_Tick(World *world, double dt)
{
    SysDebug *sys = world->systems[WORLDSYS_DEBUG];

    if (!sys || !sys->lines)
        return;
    int count = solb_count(sys->lines);
    int write = 0;
    for (int i = 0; i < count; i++)
    {
        sys->lines[i].ttl -= (float)dt;
        if (sys->lines[i].ttl >= 0)
        {
            sys->lines[write++] = sys->lines[i];
        }
    }
    solb_set_count(sys->lines, write);
}

void Debug_Draw3(World *world, double dt)
{
    SysDebug *sys = world->systems[WORLDSYS_DEBUG];
    if (!sys || !sys->lines ||solb_count(sys->lines) == 0)
        return;
    Sol_Render_DrawLine(sys->lines, solb_count(sys->lines));
}

void Debug_Draw2(World *world, double dt)
{
}

void Debug_Init(World *world)
{
    SysDebug *ws                   = malloc(sizeof(SysDebug));
    world->systems[WORLDSYS_DEBUG] = ws;
    solb_init(ws->lines, 32);
}

SolLine *Sol_Line_New(World *world)
{
    SysDebug *sys = world->systems[WORLDSYS_DEBUG];
    if (!world || !sys)
        return NULL;

    return solb_next(sys->lines);
}

void Sol_Line_Push(World *world, SolLine desc)
{
    SolLine *line = Sol_Line_New(world);
    if (line)
        *line = desc;
}
