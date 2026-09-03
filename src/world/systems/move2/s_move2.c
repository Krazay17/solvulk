#include "world.h"

void Move2_Step(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScMove2 *set = Sol_Comp_Set(world, ScMove2);
    for (int i = 0; i < set->cnt; i++)
    {
        int id = set->dense[i];
    }
}