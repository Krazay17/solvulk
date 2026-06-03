#include "sol_core.h"

void Sol_Owner_Init(World *world)
{
    world->owners = calloc(MAX_ENTS, sizeof(CompOwner));
}

void Sol_Owner_Add(World *world, int id, int ownerId)
{
    world->owners[id].ownerId = ownerId;
    world->masks[id] |= HAS_OWNER;
}

void Sol_Owner_SetTeam(World *world, int id, u32 team)
{
    world->owners[id].team = team;
}

u32 Sol_Owner_GetOwner(World *world, int id)
{
    if (world->masks[id] & HAS_OWNER)
        return world->owners[id].ownerId;
    return id;
}

u32 Sol_Owner_GetTeam(World *world, int id)
{
    if (world->masks[id] & HAS_OWNER)
        return world->owners[Sol_Owner_GetOwner(world, id)].team;
    return world->owners[id].team;
}

bool Sol_Owner_GetHostile(World *world, int id, int target)
{
    return (Sol_Owner_GetOwner(world, id) != Sol_Owner_GetOwner(world, target)) &&
           (Sol_Owner_GetTeam(world, target) == 1 || (Sol_Owner_GetTeam(world, id) != Sol_Owner_GetTeam(world, target)));
}
