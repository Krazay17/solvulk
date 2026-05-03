#include "sol_core.h"

#define EVENT_INIT_SIZE 12


void Event_Init(World *w)
{
    w->events           = malloc(sizeof(SolEvents));
    w->events->count    = 0;
    w->events->capacity = EVENT_INIT_SIZE;
    w->events->event    = calloc(w->events->capacity, sizeof(SolEvent));
}

void Sol_Event_Add(World *w, EventDesc d)
{
    SolEvents *s = w->events;
    if (!s)
        return;
    Sol_Realloc(&s->event, s->count, &s->capacity, sizeof(SolEvent));

    s->event[s->count] = (SolEvent){.entA = d.entA, .kind = d.kind, .pos = d.pos};
    s->count++;
}

void Event_Clear(World *w)
{
    if (w->events)
        w->events->count = 0;
}