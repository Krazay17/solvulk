#include "world.h"

void Score_Update(World *world, double dt)
{
    const SlEvent *e = Sol_Comp_Get(world, 0, SlEvent);
    for (int i = 0; i < solb_count(e->events); i++)
    {
        const SolEvent event = e->events[i];
        switch (event.kind)
        {
        case EVENTKIND_DEATH: {
            forc(world, event.dealer, ScScore) c->kills++;
            forc(world, event.target, ScScore) c->deaths++;
        }
        break;
        }
    }
}