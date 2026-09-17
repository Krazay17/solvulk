#include "world.h"

void Timer_Update(World *world)
{
    float fdt = world->fdt;

    SparseSet_ScTimer *set = Sol_Comp_Set(world, ScTimer);

    int count = set->cnt;
    for (int i = count - 1; i >= 0; i--)
    {
        int id         = set->dense[i];
        ScTimer *timer = &set->data[i];

        timer->elapsed += fdt;

        if (timer->elapsed > timer->duration && timer->destroy)
        {
            Sol_Destroy_Ent(world, id);
        }
    }
}