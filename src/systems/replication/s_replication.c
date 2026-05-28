#include "sol_core.h"

#include "network.h"
#include "physx/physx_i.h"
#include "replication.h"

void Sol_Replication_Init(World *world)
{
    world->replications = calloc(MAX_ENTS, sizeof(CompReplication));
    world->worldNet     = calloc(1, sizeof(WorldNet));
    memset(world->worldNet->hostToLocalMap, -1, sizeof(world->worldNet->hostToLocalMap));
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
        u32         indx = (world->worldNet->predictionCount + 1) % MAX_NET_PREDICTIONS;
        Prediction *p    = &world->worldNet->predictions[indx];
        p->localEntId    = id;
        p->prefabKind    = prefabKind;
        p->tickSpawned   = world->currentTick;
    }
}

void Sol_Replication_Disconnect(World *world)
{
    if (!world || !world->worldNet)
        return;

    WorldNet *wNet = world->worldNet;

    // 1. Loop through and clear out mapping tracking
    for (int hostId = 0; hostId <= wNet->maxHostId; hostId++)
    {
        int localId = wNet->hostToLocalMap[hostId];
        if (localId != -1)
        {
            // Destroy the entities remaining from the previous session
            Sol_Destroy_Ent(world, localId);
        }
    }

    // 2. Clear out tracking arrays completely
    memset(wNet->hostToLocalMap, -1, sizeof(wNet->hostToLocalMap));
    memset(wNet->snapShots, 0, sizeof(wNet->snapShots));
    memset(wNet->seenThisSnap, 0, sizeof(wNet->seenThisSnap));

    wNet->snapHead  = 0;
    wNet->maxHostId = 0;
}

void Net_Send_Snap(World *world)
{
    WorldSnap snap  = {0};
    snap.type       = NET_PACKET_SNAPSHOT;
    snap.worldId    = world->worldId;
    snap.tickNumber = world->currentTick;

    int count = 0;
    for (int i = 0; i < world->activeCount && count < MAX_NET_ENTS; i++)
    {
        int id = world->activeEntities[i];
        if (world->replications[id].auth != NETAUTH_AUTH)
            continue;

        NetEntityState *e = &snap.entities[count++];
        e->id             = id;
        e->compMask       = world->masks[id];
        e->prefabKind     = world->replications[id].prefabKind;
        e->pos            = world->xforms[id].pos;
        e->rot            = world->xforms[id].quat;
        e->scale          = world->xforms[id].scale.x;

        if (world->masks[id] & HAS_MODEL)
        {
            e->modelId = world->models[id].modelId;
            memcpy(e->animPlaying, world->models[id].playing, sizeof(u8) * ANIM_LAYER_COUNT);
            // e->animSeek = world->models[id].layers[ANIM_LAYER_BASE].currentSeek;
        }
        if (world->masks[id] & HAS_ABILITY)
        {
            u8           state = world->abilities[id].state;
            AbilityData *data  = &world->abilities[id].stateData[state];
            e->abilityState    = state;
            e->abilityCharge   = data->charge;
            e->abilityStage    = data->stage;
        }

        if (world->masks[id] & HAS_BODY3)
            e->vel = world->bodies[id].vel;
        if (world->masks[id] & HAS_OWNER)
        {
            e->ownerId = world->owners[id].ownerId;
            e->team    = world->owners[id].team;
        }
        if (world->masks[id] & HAS_VITAL)
        {
            e->health = world->vitals[id].health;
            e->energy = world->vitals[id].energy;
        }
    }
    snap.eCount = count; // ← outside the loop

    size_t sendSize = offsetof(WorldSnap, entities) + sizeof(NetEntityState) * count;

    ENetPacket *packet = enet_packet_create(&snap, sendSize, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_host_broadcast(solNet.host, 0, packet);
}

void Net_Apply_Snap(World *world)
{
    WorldNet *net = world->worldNet;

    // Check an explicit dynamic sequence number or an initialization flag instead of index positions
    u32        idx  = (net->snapHead + MAX_SNAPS_BUFFERED - 1) % MAX_SNAPS_BUFFERED;
    WorldSnap *snap = &net->snapShots[idx];

    // If we haven't processed any ticks yet, there's nothing to do
    if (snap->tickNumber == 0 && snap->eCount == 0)
        return;

    memset(net->seenThisSnap, 0, sizeof(net->seenThisSnap));

    for (u32 i = 0; i < snap->eCount; i++)
    {
        NetEntityState *e      = &snap->entities[i];
        int             hostId = e->id;
        if (hostId >= MAX_ENTS)
            continue;

        net->seenThisSnap[hostId] = true;
        int id                    = net->hostToLocalMap[hostId];

        if (id == -1)
        {
            if (e->prefabKind)
                id = Sol_Prefab_Factory(world, 0, e->prefabKind,
                                        (PrefabDesc){
                                            .pos       = e->pos,
                                            .rot       = e->rot,
                                            .scale     = e->scale,
                                            .authority = NETAUTH_REMOTE,
                                        });
            else
                id = Sol_Create_Ent(world, 0);

            net->hostToLocalMap[hostId] = id;
            if (hostId > net->maxHostId)
                net->maxHostId = hostId;
        }

        float dist = glms_vec3_distance2(world->xforms[id].pos, e->pos);
        float t    = 1.0f - expf(-dist / MAX_NET_INTERP_DISTANCE);
        float qt   = 1.0f - expf(-10.0f * SOL_TIMESTEP);
        if (dist < MAX_NET_INTERP_DISTANCE)
        {
            world->xforms[id].pos = glms_vec3_lerp(world->xforms[id].pos, e->pos, t);
        }
        else
        {
            world->xforms[id].pos  = e->pos;
            world->xforms[id].quat = e->rot;
        }

        world->masks[id]        = e->compMask;
        world->bodies[id].vel   = e->vel;
        world->xforms[id].quat  = glms_quat_slerp(world->xforms[id].quat, e->rot, qt);
        world->xforms[id].scale = glms_vec3_fill(e->scale);
        if (world->replications[id].auth == NETAUTH_REMOTE)
        {
            if (world->masks[id] & HAS_MODEL)
            {
                world->models[id].modelId = e->modelId;
                for (int i = 0; i < ANIM_LAYER_COUNT; i++)
                {
                    Sol_Model_PlayAnim(world, id,
                                       (AnimDesc){
                                           .anim    = e->animPlaying[i],
                                           .layerId = i,
                                       });
                }
            }
            if (world->masks[id] & HAS_ABILITY)
            {
                u8 state                   = e->abilityState;
                world->abilities[id].state = state;
                AbilityData *data          = &world->abilities[id].stateData[state];
                data->charge = e->abilityCharge;
                data->stage  = e->abilityStage;
            }
        }
        if (world->masks[id] & HAS_OWNER)
        {
            world->owners[id].ownerId = net->hostToLocalMap[e->ownerId];
            world->owners[id].team    = e->team;
        }

        if (world->masks[id] & HAS_VITAL)
        {
            world->vitals[id].health = e->health;
            world->vitals[id].energy = e->energy;
        }
    }
    // Find entities that were mapped but aren't in this snap → destroy
    for (int hostId = 0; hostId <= net->maxHostId; hostId++)
    {
        int localId = net->hostToLocalMap[hostId];
        if (localId != -1 && !net->seenThisSnap[hostId])
        {
            Sol_Destroy_Ent(world, localId);
            net->hostToLocalMap[hostId] = -1;
        }
    }
}


void Net_Send_Input(World *world)
{
    if (!(world->masks[1] & HAS_CONTROLLER))
        return;
    CompController *controller = &world->controllers[1];

    NetInputPacket input = {
        .type       = NET_PACKET_INPUT,
        .actionMask = controller->actionState,
    };
    input.lookdir     = controller->lookdir;
    input.wishdir     = controller->wishdir;
    input.aimdir      = controller->aimdir;
    input.yaw         = controller->yaw;
    input.isStrafing  = controller->isStrafing;
    input.currentTick = world->currentTick;

    ENetPacket *packet = enet_packet_create(&input, sizeof(NetInputPacket), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(solNet.peer, 0, packet);
}