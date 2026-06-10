#include "sol_core.h"

#include "network.h"

#define SNAPSHOT_INTERVAL (1.0 / 20.0) // 20Hz
#define HEARTBEAT_INTERVAL 2

SolNet        solNet;
static u32    isInitialized  = 0;
static double last_send_time = 0;
static double last_heartbeat = 0;

void Sol_Net_Tick(World **worlds, int worldCount)
{
    if (Net_IsActive())
        Net_Poll();

    if (Net_IsPlaying() && Net_IsClient())
        for (int i = 0; i < worldCount; i++)
        {
            World *world = worlds[i];
            if (world->doesReplicate)
                Net_Apply_Snap(world);
        }
}

void Sol_Net_Step(World **worlds, int count, double time)
{
    for (int w = 0; w < count; w++)
    {
        World *world = worlds[w];
        if (Net_IsPlaying() && world->doesReplicate && world->doesSimulate)
        {
            if (Net_IsClient())
            {
                Net_Send_Input(world);
            }
            else if (Net_IsHost() && Net_ShouldSend_Snap())
            {
                Net_Send_Snap(world);
                Net_Send_Events(world);
            }
        }
    }
    Net_Heartbeat(time);
}

void Net_Connect(bool host, const char *ip, u16 port)
{
    if (isInitialized == 0)
    {
        if (enet_initialize() == 0)
        {
            isInitialized = 1;
        }
        else
            return;
    }

    if (Net_IsActive())
    {
        Net_Disconnect();
    }

    ENetAddress address = {0};

    if (host)
    {
        // HOST SETUP: Listen on all local adapters on the specific game port
        solNet.role  = NETROLE_HOST;
        address.host = ENET_HOST_ANY;
        address.port = port;

        // Create a server host capable of managing 32 inbound connections
        solNet.host = enet_host_create(&address, 32, 2, 0, 0);
        printf("[Net] Server listening on port %d...\n", port);

        if (solNet.host != NULL)
        {
            solNet.status = NETSTATUS_CONNECTED;
        }
    }
    else
    {
        // CLIENT SETUP: Connect with an anonymous outward socket
        solNet.role = NETROLE_CLIENT;
        solNet.host = enet_host_create(NULL, 1, 2, 0, 0);

        // Convert string IP ("127.0.0.1") to numerical format
        enet_address_set_host(&address, ip);
        address.port = port;

        // Initiate handshaking with the host target
        solNet.peer = enet_host_connect(solNet.host, &address, 2, 0);
        printf("[Net] Client dialing %s:%d...\n", ip, port);
    }
}

void Net_DeInit()
{
    if (solNet.peer)
    {
        enet_peer_disconnect(solNet.peer, 0);
        // Wait briefly for graceful disconnect
        ENetEvent event;
        while (enet_host_service(solNet.host, &event, 1000) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                enet_packet_destroy(event.packet);
            }
        }
    }

    if (solNet.host)
    {
        enet_host_destroy(solNet.host);
        solNet.host = NULL;
    }

    enet_deinitialize();

    memset(&solNet, 0, sizeof(SolNet));
}
void Net_Disconnect()
{
    // 1. IF WE ARE THE HOST: Gracefully disconnect all inbound client peers first
    if (solNet.role == NETROLE_HOST && solNet.host != NULL)
    {
        printf("[Net] Host shutting down. Disconnecting all clients...\n");
        for (int i = 0; i < MAX_NET_CLIENTS; i++)
        {
            if (solNet.players[i].enetPeerHandle != NULL)
            {
                // Send a notification packet to this specific client peer
                enet_peer_disconnect(solNet.players[i].enetPeerHandle, 0);
            }
        }
        // Flush commands to the sockets instantly
        enet_host_flush(solNet.host);

        // Give ENet a split second to pump the outgoing disconnect packets out the door
        u32       timeout_ms = 100;
        ENetEvent event;
        while (enet_host_service(solNet.host, &event, timeout_ms) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                enet_packet_destroy(event.packet);
            }
        }
    }

    // 2. Clear out outbound client peer context if we are a client
    if (solNet.role == NETROLE_CLIENT && solNet.peer)
    {
        enet_peer_disconnect(solNet.peer, 0);
        enet_host_flush(solNet.host);
        u32       timeout_ms = 500;
        ENetEvent event;
        while (enet_host_service(solNet.host, &event, timeout_ms) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                enet_packet_destroy(event.packet);
            }
        }
        solNet.peer = NULL;
    }

    // 3. Entity cleanup tracking
    for (int i = 0; i < solNet.connectedPlayerCount; i++)
    {
        int id      = solNet.players[i].entityId;
        int worldId = solNet.players[i].currentWorldId;
        Sol_Destroy_Ent(Sol_GetWorldById(worldId), id);
        solNet.players[i].enetPeerHandle = NULL;
    }
    solNet.connectedPlayerCount = 0;

    Sol_Replication_Disconnect(Sol_GetState()->activeWorld);

    if (solNet.host)
    {
        enet_host_destroy(solNet.host);
        solNet.host = NULL;
    }

    solNet.role   = NETROLE_NONE;
    solNet.status = NETSTATUS_DISCONNECTED;
}
bool Net_ShouldSend_Snap()
{
    if (solNet.role != NETROLE_HOST)
        return false;
    if (solNet.connectedPlayerCount == 0)
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

void Net_Poll()
{
    ENetEvent event;
    while (enet_host_service(solNet.host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT: {
            printf("[Net] Connection from %x:%u\n", event.peer->address.host, event.peer->address.port);

            if (solNet.role == NETROLE_HOST)
            {
                // Host recieves connection from client
                // Find a free player slot
                for (int i = 0; i < MAX_NET_CLIENTS; i++)
                {
                    if (solNet.players[i].enetPeerHandle == NULL)
                    {
                        solNet.players[i].enetPeerHandle = event.peer;
                        solNet.players[i].currentWorldId = 0; // default world
                        solNet.connectedPlayerCount++;
                        event.peer->data = (void *)(intptr_t)i; // store slot in peer
                        printf("[Net] Assigned slot %d\n", i);
                        break;
                    }
                }
            }
            else if (solNet.role == NETROLE_CLIENT)
            {
                // Client connects to host
                NetHelloPacket hello = {
                    .type            = NET_PACKET_HELLO,
                    .worldId         = Sol_GetState()->activeWorldId,
                    .startPos        = Sol_Xform_GetPos(Sol_GetState()->activeWorld, 1),
                    .protocolVersion = SOL_VERSION,
                };
                // Player Name!
                ENetPacket *packet = enet_packet_create(&hello, sizeof(hello), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(solNet.peer, 1, packet);
                printf("[Net] Sent HELLO\n");
            }
        }
        break;

        case ENET_EVENT_TYPE_RECEIVE: {
            Net_Recv_Packet(&event);
            enet_packet_destroy(event.packet);
        }
        break;

        case ENET_EVENT_TYPE_DISCONNECT: {
            printf("[Net] Disconnect\n");

            if (solNet.role == NETROLE_HOST)
            {
                int slot = (int)(intptr_t)event.peer->data;
                Net_Remove_Player(slot);
            }
            else if (solNet.role == NETROLE_CLIENT)
            {
                Sol_Replication_Disconnect(Sol_GetState()->activeWorld);
                solNet.peer   = NULL; // <- Clear this out
                solNet.status = NETSTATUS_DISCONNECTED;
                solNet.role   = NETROLE_NONE;
            }
        }
        break;

        case ENET_EVENT_TYPE_NONE:
            break;
        }
    }
}

void Net_Remove_Player(int slot)
{
    if (slot >= 0 && slot < MAX_NET_CLIENTS)
    {
        if (solNet.players[slot].enetPeerHandle != NULL)
        {
            int id      = solNet.players[slot].entityId;
            int worldId = solNet.players[slot].currentWorldId;
            Sol_Destroy_Ent(Sol_GetWorldById(worldId), id);
            enet_peer_disconnect(solNet.players[slot].enetPeerHandle, 0);
            solNet.players[slot].enetPeerHandle = NULL;
            solNet.connectedPlayerCount--;
        }
    }
}

void Net_Recv_Packet(ENetEvent *event)
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
        if (solNet.role != NETROLE_HOST)
            return;
        NetHelloPacket *helloPacket = (NetHelloPacket *)data;
        World          *world       = Sol_GetWorldById(helloPacket->worldId);
        if (!world)
        {
            printf("No world found %d\n", helloPacket->worldId);
            return;
        }
        int id = Sol_Prefab_Factory(world, 0, EKIND_PLAYER,
                                    (EntDesc){.authority = NETAUTH_AUTH, .pos = helloPacket->startPos, .scale = 1.0f});
        if (id > 0)
            Sol_Controller_Add(world, id, CONTROLLER_REMOTE);

        int slot = (int)(intptr_t)event->peer->data;

        solNet.players[slot].entityId       = id;
        solNet.players[slot].currentWorldId = world->worldId;
        solNet.players[slot].lastPing       = Sol_GetGameTime();

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

    // Client recieves welcome from host
    case NET_PACKET_WELCOME: {
        if (solNet.role != NETROLE_CLIENT)
            return;

        NetWelcomePacket *welcomePacket                          = (NetWelcomePacket *)data;
        World            *world                                  = Sol_GetWorldById(welcomePacket->worldId);
        world->worldNet->hostToLocalMap[welcomePacket->playerId] = 1;
        Sol_GetState()->stepCounter                              = welcomePacket->currentTick;
        if (world->masks[1] & HAS_ABILITY)
        {
            for (int i = 0; i < MAX_MAPPED_SKILLS; i++)
                world->abilities[1].bindings[i].dirtySend = true;
        }

        solNet.status = NETSTATUS_CONNECTED;
    }
    break;

    case NET_PACKET_HEARTBEAT:
        solNet.players[(int)(intptr_t)event->peer->data].lastPing = Sol_GetGameTime();
        break;

    // Host recieves Input from Client
    case NET_PACKET_INPUT: {
        if (solNet.role != NETROLE_HOST)
            return;
        int             slot        = (int)(intptr_t)event->peer->data;
        int             id          = solNet.players[slot].entityId;
        NetInputPacket *inputPacket = (NetInputPacket *)data;
        World          *world       = Sol_GetWorldById(solNet.players[slot].currentWorldId);
        CompController *c           = &world->controllers[id];
        c->actionState              = inputPacket->actionMask;
        c->wishdir                  = inputPacket->wishdir;
        c->lookdir                  = inputPacket->lookdir;
        c->aimdir                   = inputPacket->aimdir;
        c->yaw                      = inputPacket->yaw;
        c->isStrafing               = inputPacket->isStrafing;
        if (inputPacket->hasEquipRequest)
        {
            for (int slot = 0; slot < MAX_MAPPED_SKILLS; slot++)
            {
                // TODO Validate here
                Sol_Ability_Bind(world, id, slot, inputPacket->abilities[slot], inputPacket->rarity[slot],
                                 inputPacket->addDamage[slot], inputPacket->addBuff[slot], inputPacket->addEffect[slot]);
            }
        }
    }
    break;
    // Host recieves Event from Client
    case NET_PACKET_EVENT: {
        if (!Net_IsPlaying())
            return;
        if (event->packet->dataLength < offsetof(EventSnap, events))
        {
            printf("[Net] Event too small\n");
            return;
        }
        EventSnap fullSnap = {0};
        size_t    copySize = event->packet->dataLength;
        if (copySize > sizeof(EventSnap))
            copySize = sizeof(EventSnap);
        memcpy(&fullSnap, event->packet->data, copySize);
        size_t expectedSize = offsetof(EventSnap, events) + sizeof(SolEvent) * fullSnap.eventCount;
        if (copySize < expectedSize)
        {
            printf("[Net] Event truncated, expected %zu, got %zu\n", expectedSize, copySize);
            return;
        }
        World *world = Sol_GetWorldById(fullSnap.worldId);
        if (!world)
        {
            printf("No world for Event Snap\n");
            return;
        }
        Net_Apply_Events(world, &fullSnap);
    }
    break;
    // Client recieves world snapshot from host
    case NET_PACKET_SNAPSHOT: {
        if (!Net_IsClient())
            return;

        if (event->packet->dataLength < offsetof(WorldSnap, entities))
        {
            printf("[Net] Snap too small\n");
            return;
        }

        // Copy into a properly-sized buffer
        static WorldSnap fullSnap;
        memset(&fullSnap, 0, sizeof(WorldSnap));
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

void Net_Heartbeat(double time)
{
    if (time < last_heartbeat + HEARTBEAT_INTERVAL)
        return;
    last_heartbeat = time;
    if (Net_IsClient())
    {
        u8          type   = NET_PACKET_HEARTBEAT;
        ENetPacket *packet = enet_packet_create(&type, 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(solNet.peer, 1, packet);
    }
    else if (Net_IsHost())
        for (int i = 0; i < solNet.connectedPlayerCount; i++)
        {
            if (solNet.players[i].enetPeerHandle == NULL)
                continue;

            if (time - solNet.players[i].lastPing > HEARTBEAT_INTERVAL * 2)
                Net_Remove_Player(i);
        }
}
