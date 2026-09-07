/*
 * File: s_debug.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Lines!
 */

#include "world.h"
#include "render/render.h"

typedef struct SysDebug
{
    DebugLine   *lines;
    DebugSphere *spheres;
} SysDebug;

void Debug_Tick(World *world, double dt)
{
    SysDebug *sys = world->systems[WORLDSYS_DEBUG];
    if (!sys)
        return;

    int i, count, write;

    count = solb_count(sys->lines);
    write = 0;
    for (i = 0; i < count; i++)
    {
        DebugLine *line = &sys->lines[i];
        line->ttl -= (float)dt;
        if (line->ttl > 0)
        {
            sys->lines[write++] = *line;
        }
    }
    solb_set_count(sys->lines, write);

    count = solb_count(sys->spheres);
    write = 0;
    for (i = 0; i < count; i++)
    {
        DebugSphere *sphere = &sys->spheres[i];
        sphere->ttl -= (float)dt;
        if (sphere->ttl > 0)
        {
            sys->spheres[write++] = *sphere;
        }
    }
    solb_set_count(sys->spheres, write);
}

void Debug_Draw3(World *world, double dt)
{
    SysDebug *sys = world->systems[WORLDSYS_DEBUG];
    if (!sys)
        return;

    if (solb_count(sys->lines) > 0)
    {
        Sol_Render_DrawLines(&sys->lines[0].line, solb_count(sys->lines), sizeof(DebugLine));
    }

    // sollog(solb_count(sys->spheres));
    for (int i = 0; i < solb_count(sys->spheres); i++)
    {
        SolSphere *dsphere = &sys->spheres[i].sphere;

        SphereSSBO *sphere = Sol_Render_GetNextSphere(SPHEREKIND_DEBUG);
        sphere->color      = dsphere->color;
        sphere->pos        = (vec4s){ dsphere->pos.x, dsphere->pos.y, dsphere->pos.z, dsphere->radius };
    }
}

void Debug_Draw2(World *world, double dt)
{
}

void Debug_Init(World *world)
{
    SysDebug *ws                   = malloc(sizeof(SysDebug));
    world->systems[WORLDSYS_DEBUG] = ws;
    solb_init(ws->lines, 32);
    solb_init(ws->spheres, 32);
}

SolLine *Sol_Debug_NewLine(World *world, float ttl)
{
    SysDebug *sys = world->systems[WORLDSYS_DEBUG];
    if (!world || !sys)
        return NULL;

    DebugLine *dline = solb_next(sys->lines);
    dline->ttl       = ttl;
    return &dline->line;
}

void Sol_Line_Push(World *world, SolLine desc, float ttl)
{
    SolLine *line = Sol_Debug_NewLine(world, ttl);
    if (line)
        *line = desc;
}

SolSphere *Sol_Debug_NewSphere(World *world, float ttl)
{
    SysDebug *sys = world->systems[WORLDSYS_DEBUG];
    if (!world || !sys)
        return NULL;

    DebugSphere *dsphere = solb_next(sys->spheres);
    dsphere->ttl         = ttl;
    return &dsphere->sphere;
}