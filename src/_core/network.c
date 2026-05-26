#include "network.h"
#include "sol_core.h"

#include "controller/controller_i.h"
#include "vital/vital_i.h"

#define SNAPSHOT_INTERVAL (1.0 / 20.0) // 20Hz

static double last_send_time = 0;

void Net_Init(SolNet *net, bool host, const char *ip, u16 port)
{

    if (enet_initialize() != 0)
        printf("enet failed");
    net->isHost      = host;
    net->isConnected = false;

    ENetAddress address = {0};

    if (net->isHost)
    {
        // HOST SETUP: Listen on all local adapters on the specific game port
        address.host = ENET_HOST_ANY;
        address.port = port;

        // Create a server host capable of managing 32 inbound connections
        net->host = enet_host_create(&address, 32, 2, 0, 0);
        printf("[Net] Server listening on port %d...\n", port);
    }
    else
    {
        // CLIENT SETUP: Connect with an anonymous outward socket
        net->host = enet_host_create(NULL, 1, 2, 0, 0);

        // Convert string IP ("127.0.0.1") to numerical format
        enet_address_set_host(&address, ip);
        address.port = port;

        // Initiate handshaking with the host target
        net->peer = enet_host_connect(net->host, &address, 2, 0);
        printf("[Net] Client dialing %s:%d...\n", ip, port);
    }

    if (net->host == NULL)
    {
        printf("[Net Error] Failed to create ENet host context.\n");
    }
}

void Net_DeInit(SolNet *net)
{
    if (!net->isHost && net->peer)
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
    if (!world->worldNet)
        return 1;
    return 0;
}

bool Net_ShouldSend(SolNet *net)
{
    if (!net->isHost)
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
    if (!net->host)
        return;

    ENetEvent event;
    while (enet_host_service(net->host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT: {
            printf("[Net] Connection from %x:%u\n", event.peer->address.host, event.peer->address.port);

            if (net->isHost)
            {
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
            else
            {
                // Client connect to server
                net->state = NET_STATE_AWAITING_WELCOME;

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

            if (net->isHost)
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
                net->isConnected = false;
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
    case NET_PACKET_HELLO: {
        if (!net->isHost)
            return;
        NetHelloPacket *helloPacket = (NetHelloPacket *)data;
        World          *world       = Sol_GetWorldById(helloPacket->worldId);
        if (!world)
        {
            printf("No world found %d\n", helloPacket->worldId);
            return;
        }
        int id = Sol_Prefab_Player(world,0, helloPacket->startPos, 1.0f);

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

    case NET_PACKET_WELCOME: {
        if (net->isHost)
            return;
        NetWelcomePacket *welcomePacket       = (NetWelcomePacket *)data;
        LocalPlayer      *localPlayer         = &Sol_GetState()->localPlayer;
        World *world = Sol_GetWorldById(welcomePacket->worldId);
        //Sol_World
        *localPlayer->playerId = welcomePacket->playerId;
    }
    break;

    case NET_PACKET_REJECT: {
    }
    break;

    case NET_PACKET_SNAPSHOT: {
        NetSnapshotPacket *snap = (NetSnapshotPacket *)data;
        for (int i = 0; i < Sol_GetState()->worldCount; i++)
        {
            World *world = Sol_GetState()->worlds[i];
            if (world->worldId == snap->worldId)
            {
                // Store in world's snap buffer
                world->worldNet->snapShots[world->worldNet->snapHead];
                world->worldNet->snapHead = (world->worldNet->snapHead + 1) % MAX_SNAPS_BUFFERED;
                break;
            }
        }
    }
    break;

    case NET_PACKET_INPUT: {
        // Server receives input from a client
        // Apply to the player entity owned by this peer
        int slot = (int)(intptr_t)event->peer->data;
        net->players[slot].currentWorldId;
    }
    break;
    }
}

void Net_Send_Snap(SolNet *net, World *world)
{
    if (!net->isHost || !world->doesReplicate)
        return;
    NetSnapshotPacket snapPacket = (NetSnapshotPacket){.type = NET_PACKET_SNAPSHOT, .worldId = world->worldId};

    snapPacket.snap.tickNumber = world->currentTick;
    snapPacket.snap.worldId    = world->worldId;

    int count = 0;
    for (int i = 0; i < world->activeCount && count < MAX_NET_ENTS; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & HAS_XFORM))
            continue;

        NetEntityState *e = &snapPacket.snap.entities[count++];
        e->id             = id;
        e->compMask       = world->masks[id];
        e->pos            = world->xforms[id].drawPos;
        e->rot            = world->xforms[id].drawQuat;

        if (world->masks[id] & HAS_VITAL)
        {
            e->health = world->vitals[id].health;
            e->energy = world->vitals[id].energy;
        }
        if (world->masks[id] & HAS_CONTROLLER)
        {
            e->inputs = world->controllers[id].actionState;
        }
    }
    snapPacket.snap.eCount = count;

    ENetPacket *packet = enet_packet_create(&snapPacket, sizeof(NetSnapshotPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(net->host, 0, packet);
}

void Net_Apply_Snap(World *world)
{
    if (world->worldNet->snapHead == 0)
        return;

    // Latest snap is at head-1 (modular)
    u32        idx  = (world->worldNet->snapHead + MAX_SNAPS_BUFFERED - 1) % MAX_SNAPS_BUFFERED;
    WorldSnap *snap = &world->worldNet->snapShots[idx];
    // printf("Tick: %d\n",snap->tickNumber);

    for (u32 i = 0; i < snap->eCount; i++)
    {
        NetEntityState *e  = &snap->entities[i];
        int             id = e->id;
        if (id >= MAX_ENTS)
            continue;

        // If entity doesn't exist locally yet, spawn it
        if (!world->activeEntities[id])
        {
            world->masks[id] = e->compMask;
        }

        // Apply position
        if (world->masks[id] & HAS_XFORM)
        {
            world->xforms[id].pos     = e->pos;
            world->xforms[id].drawPos = e->pos; // skip interp for now
        }

        // Apply vitals
        if (world->masks[id] & HAS_VITAL)
        {
            world->vitals[id].health = e->health;
            world->vitals[id].energy = e->energy;
        }
    }
}