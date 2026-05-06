#include "sol_core.h"

typedef struct CompTimer
{
    float elapsed, duration;
} CompTimer;

void Sol_Timer_Init(World *world)
{
    world->timers = calloc(MAX_ENTS, sizeof(CompTimer));
}

void Sol_Timer_Add(World *world, int id, TimerDesc desc)
{
    CompTimer timer = {
        .duration = desc.duration,
        .elapsed = desc.elapsed,
    };

    world->timers[id] = timer;
    world->masks[id] |= HAS_TIMER;
    return &world->timers[id];
}

void Sol_Timer_Tick(World *world, double dt, double time)
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