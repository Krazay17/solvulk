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

        // if (Sol_Comp_Has(world, id, ScInteract))
        // {
        //     ScInteract *interact = Sol_Comp_Get(world, id, ScInteract);
        //     if (interact->state & INTERACT_DOWN)
        //     {
        //         if (hook->held)
        //             hook->held(world, id, dt, interact->interactors, hook->data);
        //     }
        //     if (interact->state & INTERACT_JUSTUP)
        //     {
        //         if (hook->pressed)
        //             hook->pressed(world, id, dt, interact->interactors, hook->data);
        //     }
        // }
    }
}
