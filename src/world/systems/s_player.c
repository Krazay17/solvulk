#include "world.h"
#include "input.h"
#include "sol_user.h"
#include "sol_math.h"

void Player_Tick(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScPlayer *set = Sol_Comp_Set(world, ScPlayer);
    for (int i = 0; i < set->cnt; i++)
    {
        int            id     = set->dense[i];
        ScPlayer     *player = &set->data[i];
    }
}