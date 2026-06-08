/*
 * File: s_event.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Events!
 */
#include "sol_core.h"

#include "event.h"

#define MAX_EVENTS 65536

void Sol_Event_Init(World *w)
{
    w->events           = malloc(sizeof(SolEvents));
    w->events->count    = 0;
    w->events->capacity = MAX_EVENTS;
    w->events->event    = calloc(w->events->capacity, sizeof(SolEvent));
}

void Sol_Event_Add(World *w, SolEvent d)
{
    SolEvents *s = w->events;
    if (!s)
        return;
    if (s->count >= s->capacity)
        return;

    s->event[s->count] = d;
    s->count++;
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