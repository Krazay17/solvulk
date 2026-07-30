#include "s_container.h"
#include "sol/sol.h"

void Sol_Container_Init(World *world)
{
    world->containers      = malloc(sizeof(CompContainer));
    world->containers->cnt = 0;
}

void Sol_Container_Add(World *world, int id)
{
    CompContainer *container = &world->containers[id];
    
}
