/*
 * File: s_hook.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-03
 *
 */

#include "world.h"

void Hook_Tick(World *world, double dt)
{
    SparseSet_ScHook *set = Sol_Comp_Set(world, ScHook);
    for (int i = 0; i < set->cnt; i++)
    {
        int id       = set->dense[i];
        ScHook *hook = &set->data[i];

        if (hook->update)
            hook->update(world, id, 0, dt, hook->data);
    }
}
