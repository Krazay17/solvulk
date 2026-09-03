/*
 * File: s_interface.c
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
        int     id   = set->dense[i];
        ScHook *hook = &set->data[i];

        if (hook->update)
            hook->update(world, dt, id, hook->data);

        if (Sol_Comp_Has(world, id, ScInteract))
        {
            ScInteract *interact = Sol_Comp_Get(world, id, ScInteract);
            if (interact->state & INTERACT_HELD)
            {
                if (hook->held)
                    hook->held(world, dt, id, hook->data);
            }
            if (interact->state & INTERACT_PRESSED)
            {
                if (hook->pressed)
                    hook->pressed(world, dt, id, hook->data);
            }
        }
    }
}