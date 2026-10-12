#include "world.h"

void Event_Update(World *world, double dt)
{
    forc(world, 0, SlEvent)
    {
        for (int i = 0; i < solb_count(c->events); i++)
        {
            SolEvent *e            = &c->events[i];
            EventHandler *handlers = c->handlers[e->kind];
            for (int j = 0; j < solb_count(handlers); j++)
            {
                if (handlers[j])
                    handlers[j](world, e);
            }
        }
    }
}

Sol_Event_AddHandler(World *world, EventKind kind, EventHandler handler)
{
    forc(world, 0, SlEvent) solb_push(c->handlers[kind], handler);
}