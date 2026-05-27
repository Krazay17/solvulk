#include "network.h"
#include "sol_core.h"

#include "physx/physx_i.h"

#define SNAPSHOT_INTERVAL (1.0 / 20.0) // 20Hz
#define MAX_NET_INTERP_DISTANCE 0.2f

static double last_send_time = 0;

void Net_Init(SolNet *net, bool host, const char *ip, u16 port)
{

    if (enet_initialize() != 0)
        printf("enet failed");

    ENetAddress address = {0};

    if (host)
    {
        // HOST SETUP: Listen on all local adapters on the specific game port
        net->role    = NETROLE_HOST;
        address.host = ENET_HOST_ANY;
        address.port = port;

        // Create a server host capable of managing 32 inbound connections
        net->host = enet_host_create(&address, 32, 2, 0, 0);
        printf("[Net] Server listening on port %d...\n", port);

        if (net->host != NULL)
        {
            net->status = NETSTATUS_CONNECTED;
        }
    }
    else
    {
        // CLIENT SETUP: Connect with an anonymous outward socket
        net->role = NETROLE_CLIENT;
        net->host = enet_host_create(NULL, 1, 2, 0, 0);

        // Convert string IP ("127.0.0.1") to numerical format
        enet_address_set_host(&address, ip);
        address.port = port;

        // Initiate handshaking with the host target
        net->peer = enet_host_connect(net->host, &address, 2, 0);
        printf("[Net] Client dialing %s:%d...\n", ip, port);
    }
}

void Net_DeInit(SolNet *net)
{
    if (net->peer)
    {
        enet_peer_disconnect(net->peer, 0);
        // Wait briefly for graceful disconnect
        ENetEvent event;
        while (enet_host_service(net->host, &event, 1000) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                enet_packet_destroy(event.packet);
            }
        }
    }

    if (net->host)
    {
        enet_host_destroy(net->host);
        net->host = NULL;
    }

    enet_deinitialize();

    memset(net, 0, sizeof(SolNet));
}

int Net_World_Init(World *world)
{
    world->worldNet = calloc(1, sizeof(WorldNet));
    memset(world->worldNet->hostToLocalMap, -1, sizeof(world->worldNet->hostToLocalMap));
    if (!world->worldNet)
        return 1;
    return 0;
}

bool Net_ShouldSend_Snap(SolNet *net)
{
    if (net->role != NETROLE_HOST)
        return false;
    if (net->connectedPlayerCount == 0)
        return false;

    // Send at tick for now
    return true;

    double now = Sol_GetGameTime();
    if (now - last_send_time >= SNAPSHOT_INTERVAL)
    {
        last_send_time = now;
        return true;
    }
    return false;
}

void Net_Poll(SolNet *net)
{
    ENetEvent event;
    while (enet_host_service(net->host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT: {
            // printf("[Net] Connection from %x:%u\n", event.peer->address.host, event.peer->address.port);

            if (net->role == NETROLE_HOST)
            {
                // Host recieves connection from client
                // Find a free player slot
                for (int i = 0; i < MAX_NET_CLIENTS; i++)
                {
                    if (net->players[i].enetPeerHandle == NULL)
                    {
                        net->players[i].enetPeerHandle = event.peer;
                        net->players[i].currentWorldId = 0; // default world
                        net->connectedPlayerCount++;
                        event.peer->data = (void *)(intptr_t)i; // store slot in peer
                        printf("[Net] Assigned slot %d\n", i);
                        break;
                    }
                }
            }
            else if (net->role == NETROLE_CLIENT)
            {
                // Client connects to host
                NetHelloPacket hello = {
                    .type            = NET_PACKET_HELLO,
                    .worldId         = Sol_GetState()->localPlayer.worldId,
                    .startPos        = Sol_Xform_GetPos(Sol_GetState()->localPlayer.activeWorld,
                                                        Sol_GetState()->localPlayer.activeWorld->playerID),
                    .protocolVersion = SOL_VERSION,
                };
                strncpy(hello.name, Sol_GetState()->localPlayer.playerName, sizeof(hello.name));

                ENetPacket *packet = enet_packet_create(&hello, sizeof(hello), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(net->peer, 1, packet);
                printf("[Net] Sent HELLO\n");
            }
        }
        break;

        case ENET_EVENT_TYPE_RECEIVE: {
            Net_Recv_Packet(net, &event);
            enet_packet_destroy(event.packet);
        }
        break;

        case ENET_EVENT_TYPE_DISCONNECT: {
            printf("[Net] Disconnect\n");

            if (net->role == NETROLE_HOST)
            {
                int slot = (int)(intptr_t)event.peer->data;
                if (slot >= 0 && slot < MAX_NET_CLIENTS)
                {
                    net->players[slot].enetPeerHandle = NULL;
                    net->connectedPlayerCount--;
                }
            }
            else
            {
                net->status = NETSTATUS_DISCONNECTED;
            }
            event.peer->data = NULL;
        }
        break;

        case ENET_EVENT_TYPE_NONE:
            break;
        }
    }
}

void Net_Recv_Packet(SolNet *net, ENetEvent *event)
{
    if (event->packet->dataLength < 1)
    {
        printf("[Net] Packet too small\n");
        return;
    }

    u8   *header = (u8 *)event->packet->data;
    void *data   = event->packet->data;

    switch (*header)
    {
    // Host recieves hello from client
    case NET_PACKET_HELLO: {
        if (net->role != NETROLE_HOST)
            return;
        NetHelloPacket *helloPacket = (NetHelloPacket *)data;
        World          *world       = Sol_GetWorldById(helloPacket->worldId);
        if (!world)
        {
            printf("No world found %d\n", helloPacket->worldId);
            return;
        }
        int id =
            Sol_Prefab_Factory(world, 0, PREFABKIND_PLAYER,
                               (PrefabDesc){.authority = NETAUTH_AUTH, .pos = helloPacket->startPos, .scale = 1.0f});
        Sol_Controller_Add(world, id, CONTROLLER_REMOTE);

        int slot = (int)(intptr_t)event->peer->data;

        net->players[slot].entityId       = id;
        net->players[slot].currentWorldId = world->worldId;

        NetWelcomePacket welcomePacket = {
            .type        = NET_PACKET_WELCOME,
            .currentTick = Sol_GetState()->tickCounter,
            .playerId    = id,
            .worldId     = world->worldId,
        };
        ENetPacket *packet = enet_packet_create(&welcomePacket, sizeof(welcomePacket), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(event->peer, 1, packet);
    }
    break;
    // Host recieves Input from Client
    case NET_PACKET_INPUT: {
        if (net->role != NETROLE_HOST)
            return;
        int             slot        = (int)(intptr_t)event->peer->data;
        int             id          = net->players[slot].entityId;
        NetInputPacket *inputPacket = (NetInputPacket *)data;
        World          *world       = Sol_GetWorldById(net->players[slot].currentWorldId);
        CompController *c           = &world->controllers[id];
        c->actionState              = inputPacket->actionMask;
        c->wishdir                  = inputPacket->wishdir;
        c->lookdir                  = inputPacket->lookdir;
        c->aimdir                   = inputPacket->aimdir;
        c->yaw                      = inputPacket->yaw;
        c->isStrafing               = inputPacket->isStrafing;
    }
    break;
    // Host recieves Event from Client
    case NET_PACKET_EVENT: {
    }
    break;
    // Client recieves welcome from host
    case NET_PACKET_WELCOME: {
        if (net->role != NETROLE_CLIENT)
            return;

        NetWelcomePacket *welcomePacket = (NetWelcomePacket *)data;
        Sol_GetWorldById(welcomePacket->worldId)->worldNet->hostToLocalMap[welcomePacket->playerId] = 1;

        net->status = NETSTATUS_CONNECTED;
    }
    break;
    // Client recieves world snapshot from host
    case NET_PACKET_SNAPSHOT: {
        if (net->role != NETROLE_CLIENT || net->status != NETSTATUS_CONNECTED)
            return;

        if (event->packet->dataLength < offsetof(WorldSnap, entities))
        {
            printf("[Net] Snap too small\n");
            return;
        }

        // Copy into a properly-sized buffer
        WorldSnap fullSnap = {0};
        size_t    copySize = event->packet->dataLength;
        if (copySize > sizeof(WorldSnap))
        {
            copySize = sizeof(WorldSnap); // clamp
        }
        memcpy(&fullSnap, event->packet->data, copySize);

        // Validate
        if (fullSnap.eCount > MAX_NET_ENTS)
        {
            printf("[Net] Bad eCount: %u\n", fullSnap.eCount);
            return;
        }

        size_t expectedSize = offsetof(WorldSnap, entities) + sizeof(NetEntityState) * fullSnap.eCount;
        if (event->packet->dataLength < expectedSize)
        {
            printf("[Net] Snap truncated: got %zu expected %zu\n", (size_t)event->packet->dataLength, expectedSize);
            return;
        }

        // Find the world and store
        World *world = Sol_GetWorldById(fullSnap.worldId);
        if (!world)
        {
            printf("[Net] Unknown worldId %u\n", fullSnap.worldId);
            return;
        }

        WorldNet *worldNet                      = world->worldNet;
        worldNet->snapShots[worldNet->snapHead] = fullSnap;
        worldNet->snapHead                      = (worldNet->snapHead + 1) % MAX_SNAPS_BUFFERED;
    }
    break;

    case NET_PACKET_REJECT: {
    }
    break;
    }
}
void Net_Send_Snap(SolNet *net, World *world)
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
    enet_host_broadcast(net->host, 0, packet);
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

        int id = net->hostToLocalMap[hostId];

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
            world->masks[id]            = e->compMask;
            if (hostId > net->maxHostId)
                net->maxHostId = hostId;
        }

        float t    = fminf(fmaxf(SOL_TIMESTEP / 0.05f, 0.0f), 1.0f);
        float dist = glms_vec3_distance2(world->xforms[id].pos, e->pos);
        if (dist < MAX_NET_INTERP_DISTANCE)
        {
            world->xforms[id].pos = glms_vec3_lerp(world->xforms[id].pos, e->pos, t);
        }
        else
        {
            world->xforms[id].pos  = e->pos;
            world->xforms[id].quat = e->rot;
        }
        world->xforms[id].quat  = glms_quat_slerp(world->xforms[id].quat, e->rot, t);
        world->xforms[id].scale = glms_vec3_fill(e->scale);
        world->bodies[id].vel   = e->vel;

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
    // for (int i = 0; i < net->predictionCount; i++)
    // {
    //     Prediction *p = &net->predictions[i];
    //     if (p->reconciled)
    //         continue;
    //     if (world->currentTick - p->tickSpawned > 30)
    //     { // ~500ms at 60Hz
    //         // Old prediction, no match arrived — destroy the local
    //         Sol_Destroy_Ent(world, p->localEntId);
    //         // Mark as cleaned up
    //     }
    // }
}

void Net_Send_Input(SolNet *net, World *world)
{
    if (net->role != NETROLE_CLIENT || net->status != NETSTATUS_CONNECTED)
        return;
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
    enet_peer_send(net->peer, 0, packet);
}

// int TryMatchPrediction(World *world, NetEntityState *e)
// {
//     WorldNet *net = world->worldNet;
//     for (int i = 0; i < net->predictionCount; i++)
//     {
//         Prediction *p = &net->predictions[i];
//         if (p->reconciled)
//             continue;
//         if (p->prefabKind != e->prefabKind)
//             continue;

//         vec3s localPos = world->xforms[p->localEntId].pos;
//         if (glms_vec3_distance(localPos, e->pos) > 5.0f)
//             continue;

//         // Match!
//         p->reconciled = true;
//         return p->localEntId;
//     }
//     return -1;
// }