#include "sol_core.h"

void Sol_Replication_Init(World *world)
{
    world->replications = calloc(MAX_ENTS, sizeof(CompReplication));
}

void Sol_Replication_Add(World *world, int id, NetRole role, u8 prefabKind)
{
    CompReplication rep = {
        .role       = role,
        .prefabKind = prefabKind,
    };
    world->masks[id] |= HAS_REPLICATION;
    world->replications[id] = rep;
}