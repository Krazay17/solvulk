#include "game.h"
#include "world.h"

static World world;

void Create_Sol_Game()
{
    Sol_World_InitAllComponents(&world, 512);
    while (1)
    {
        sollog(world.maxEntities);
        bool hasXform = Sol_Comp_Has(&world, 0, SolXform);
        sollog(hasXform);
    }
}