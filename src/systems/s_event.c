#include "world.h"

void Event_Clear(World *world, double dt)
{
    SlEvent *events = Sol_Comp_Get(world, 0, SlEvent);
    solb_set_count(events->events, 0);
}

void Event_Step(World *world, double dt)
{
    forc(world, 0, SlEvent)
    {
        for (int i = 0; i < solb_count(c->events); i++)
        {
            SolEvent *e           = &c->events[i];
            EventHandler *handler = c->handlers[e->kind];

            for (int j = 0; j < solb_count(handler); j++)
            {
                if (handler[j])
                    handler[j](world, e);
            }
        }
    }
}

void Sol_Event_Push(World *world, EventKind kind, SolEvent event)
{
    SlEvent *single = Sol_Comp_Get(world, 0, SlEvent);
    event.kind      = kind;
    solb_push(single->events, event);
}

void Sol_Event_AddHandler(World *world, EventKind kind, EventHandler handler)
{
    forc(world, 0, SlEvent) solb_push(c->handlers[kind], handler);
}
