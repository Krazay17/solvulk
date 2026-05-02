#include "event.h"
#include "sol_core.h"

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
    if (s->count >= s->capacity)
    {
        s->capacity *= 2;
        s->event = realloc(s->event, sizeof(SolEvent) * s->capacity);
    }
    u32 index = s->count++;

    SolEvent *e = &s->event[index];
    *e          = (SolEvent){.entA = d.entA, .kind = d.kind, .pos = d.pos};
}

void Event_Clear(World *w)
{
    if (w->events)
        w->events->count = 0;
}