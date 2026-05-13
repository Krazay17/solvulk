#include "sol_core.h"

void Sol_Combat_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Combat_Step;
}

void Sol_Combat_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e = &world->events->event[i];
        switch (e->kind)
        {
        case EVENT_COLLISION:
            Sol_Emitter_Add(world, (Emitter){.burst    = 10,
                                             .pos      = e->as.collision.pos,
                                             .particle = (Particle){.ttl = 0.5f, .scale = 0.25f}});
        }
    }
}