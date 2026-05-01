#include "sol_core.h"

CompTimer *Sol_Timer_Add(World *world, int id, CompTimer init)
{
    CompTimer timer = init;

    world->timers[id] = timer;
    world->masks[id] |= HAS_TIMER;
    return &world->timers[id];
}

void Timer_Tick(World *world, double dt, double time)
{
    int required = HAS_TIMER;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompTimer *timer = &world->timers[id];
        timer->elapsed += dt;

        if (timer->elapsed >= timer->duration)
        {
            Sol_Destroy_Ent(world, id);
        }
    }
}