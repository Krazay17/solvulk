#include "world.h"
#include "input.h"
#include "sol_user.h"
#include "sol_math.h"

void Player_Tick(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_SolPlayer *set = Sol_Comp_Set(world, SolPlayer);
    for (int i = 0; i < set->cnt; i++)
    {
        int            id     = set->dense[i];
        SolPlayer     *player = &set->data[i];
    }
}