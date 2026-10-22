#include "world.h"

void Pickup_Update(World *world, double dt)
{
    SolRayResult results[8];
    for_set(world, ScPickup)
    {
        int id       = set->dense[i];
        SolItem item = c->item;
        int hits     = Sol_SphereOverlap(
            world, (SolRay){.ignoreEnt = id, .radius = c->range, .start = world->xform.pos[id]}, results, 8);
        for (int j = 0; j < hits; j++)
        {
            forc(world, results[j].entId, ScInventory)
            {
                Sol_Inv_AddItem(world, results[j].entId, &item, 0);
                Sol_Destroy_Ent(world, id);
                goto next_query;
            }
        }
        next_query:;
    }
}