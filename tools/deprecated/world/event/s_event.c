/*
 * File: s_event.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Events!
 */
#include "s_event.h"
#include "world.h"
#include "sol_core.h"

void Sol_Event_Init(World *w)
{
    w->events           = malloc(sizeof(SolEvents));
    w->events->event    = NULL;
    w->events->count    = 0;
    w->events->capacity = 0;
}

void Sol_Event_Add(World *w, SolEvent d)
{
    SolEvents *s = w->events;
    if (!s)
        return;
    if (Sol_Realloc((void **)&s->event, s->count, &s->capacity, sizeof(SolEvent)) != 0)
        return;

    s->event[s->count++] = d;
}

void Sol_Events_Clear(World **worlds, int count)
{
    for (int w = 0; w < count; w++)
    {
        World *world         = worlds[w];
        world->events->count = 0;
    }
}

void Sol_Event_Clear(World *w)
{
    if (w->events)
        w->events->count = 0;
}

int Sol_Event_GetCount(World *w)
{
    return w->events->count;
}

SolEvent Sol_Event_GetEvent(World *w, int eventId)
{
    return w->events->event[eventId];
}