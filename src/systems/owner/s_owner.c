#include "sol_core.h"

void Sol_Owner_Init(World *world)
{
    world->owners = calloc(MAX_ENTS, sizeof(CompOwner));
}

void Sol_Owner_Add(World *world, int id, int ownerId)
{
    world->masks[id] |= HAS_OWNER;
    world->owners[id].ownerId = ownerId;
}

u32 Sol_Owner_GetOwner(World *world, int id)
{
    if (world->masks[id] & HAS_OWNER)
        return world->owners[id].ownerId;
    return id;
}