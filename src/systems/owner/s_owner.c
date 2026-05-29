#include "sol_core.h"

void Sol_Owner_Init(World *world)
{
    world->owners = calloc(MAX_ENTS, sizeof(CompOwner));
}

void Sol_Owner_Add(World *world, int id, OwnerDesc desc)
{
    world->masks[id] |= HAS_OWNER;
    CompOwner *owner = &world->owners[id];
    owner->ownerId   = desc.ownerId ? desc.ownerId : id;
    owner->team      = desc.team;
}

u32 Sol_Owner_GetOwner(World *world, int id)
{
    if (world->masks[id] & HAS_OWNER)
        return world->owners[id].ownerId;
    return id;
}

void Sol_Owner_SetOwner(World *world, int id, int ownerId)
{
    world->masks[id] |= HAS_OWNER;
    CompOwner *owner = &world->owners[id];
    owner->ownerId   = ownerId;
    owner->team      = Sol_Owner_GetTeam(world, ownerId);
}

u32 Sol_Owner_GetTeam(World *world, int id)
{
    return world->owners[id].team;
}

bool Sol_Owner_GetHostile(World *world, int id, int target)
{
    return id != target && (Sol_Owner_GetTeam(world, target) == 1 || Sol_Owner_GetTeam(world, id) != Sol_Owner_GetTeam(world, target));
}