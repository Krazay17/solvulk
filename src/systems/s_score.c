#include "world.h"

static void AddDamage(World *world, int id, float damage)
{
    ScScore *score = Sol_Comp_Add(world, id, ScScore);
    if(score)
    {
        score->damage += damage;
    }
}

static void AddKill(World *world, int id)
{
    ScScore *score = Sol_Comp_Add(world, id, ScScore);
    if(score)
    {
        score->kills++;
    }
}

static void AddDeath(World *world, int id)
{
    ScScore *score = Sol_Comp_Add(world, id, ScScore);
    if(score)
    {
        score->deaths++;
    }
}

void Score_Update(World *world, double dt)
{
    const SlEvent *e = Sol_Comp_Get(world, 0, SlEvent);
    for (int i = 0; i < solb_count(e->events); i++)
    {
        const SolEvent event = e->events[i];
        switch (event.kind)
        {
        case EVENTKIND_DEATH: {
            AddKill(world, event.dealer);
            AddDeath(world, event.target);
        }
        case EVENTKIND_HIT: {
            AddDamage(world, event.dealer, event.as.hit.amount);
        }
        break;
        }
    }
}
