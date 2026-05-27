#include "sol_core.h"

void Sol_Replication_Init(World *world)
{
    world->replications = calloc(MAX_ENTS, sizeof(CompReplication));
}

void Sol_Replication_Add(World *world, int id, NetAuth auth, u8 prefabKind)
{
    CompReplication rep = {
        .auth       = auth,
        .prefabKind = prefabKind,
    };
    world->masks[id] |= HAS_REPLICATION;
    world->replications[id] = rep;

    if (auth == NETAUTH_LOCAL)
    {
        if (world->worldNet->predictionCount <= MAX_NET_PREDICTIONS)
            return;
        Prediction *p  = &world->worldNet->predictions[world->worldNet->predictionCount++];
        p->localEntId  = id;
        p->prefabKind  = prefabKind;
        p->tickSpawned = world->currentTick;
    }
}