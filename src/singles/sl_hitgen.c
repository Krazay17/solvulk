#include "world.h"

void Sl_Hitgen_Init(World *world)
{
    SlHitgen *single              = malloc(sizeof(SlHitgen));
    world->singles[SINGLE_HITGEN] = single;
    memset(single->matrix, 0, sizeof(single->matrix));
    single->global = 1;
}

u32 Sol_Hitgen_Start(World *world, int id)
{
    SlHitgen *single = world->singles[SINGLE_HITGEN];

    single->global++;
    if (single->global == 0)
    {
        memset(single->matrix, 0, sizeof(single->matrix));
        single->global = 1;
    }
    return single->global;
}

bool Sol_Hitgen_Try(World *world, int id, int target, u32 sessionGen)
{
    SlHitgen *single = world->singles[SINGLE_HITGEN];

    if (single->matrix[id][target] == sessionGen)
        return false;

    single->matrix[id][target] = sessionGen;

    return true;
}
