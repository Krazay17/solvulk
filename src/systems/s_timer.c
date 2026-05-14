/*
 * File: s_timer.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Timer!
 */
#include "sol_core.h"

typedef struct CompTimer
{
    float elapsed, duration;
} CompTimer;

void Sol_Timer_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Timer_Step;

    world->timers = calloc(MAX_ENTS, sizeof(CompTimer));
}

void Sol_Timer_Add(World *world, int id, TimerDesc desc)
{
    CompTimer timer = {
        .duration = desc.duration,
        .elapsed  = desc.elapsed,
    };

    world->timers[id] = timer;
    world->masks[id] |= HAS_TIMER;
}

void Sol_Timer_Step(World *world, double dt, double time)
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