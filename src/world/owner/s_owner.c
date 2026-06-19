#include "s_owner.h"
#include "world.h"

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
// 1. Resolve absolute root entities down to their true controllers
    u32 rootA = Sol_Owner_GetOwner(world, id);
    u32 rootB = Sol_Owner_GetOwner(world, target);

    // 2. If the root controllers match, it's a self-hit or teammate-hit
    if (rootA == rootB)
    {
        return false;
    }

    // 3. Now perform faction checks safely on the resolved root targets
    u32 teamA = world->owners[rootA].team;
    u32 teamB = world->owners[rootB].team;

    return (teamB == 1) || (teamA != teamB);
}
