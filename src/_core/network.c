#include "network.h"
#include "sol_core.h"

#include "controller/controller_i.h"
#include "physx/physx_i.h"
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
    memset(world->worldNet->hostToLocalMap, -1, sizeof(world->worldNet->hostToLocalMap));
    if (!world->worldNet)
        return 1;
    return 0;
}

bool Net_ShouldSend_Snap(SolNet *net)
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
        int id   = Sol_Prefab_Player(world, 0, helloPacket->startPos, 1.0f);
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

    case NET_PACKET_WELCOME: {
        if (net->isHost)
            return;
        net->state = NET_STATE_PLAYING;
    }
    break;

    case NET_PACKET_REJECT: {
    }
    break;

    case NET_PACKET_SNAPSHOT: {
        if (net->isHost)
            return;
        NetSnapshotPacket *snap = (NetSnapshotPacket *)data;
        for (int i = 0; i < Sol_GetState()->worldCount; i++)
        {
            World *world = Sol_GetState()->worlds[i];
            if (world->worldId == snap->worldId)
            {
                WorldNet *net = world->worldNet;

                // FIX: Explicitly assign the incoming struct payload data right into the slot!
                net->snapShots[net->snapHead] = snap->snap;

                // Advance the ring buffer head safely
                net->snapHead = (net->snapHead + 1) % MAX_SNAPS_BUFFERED;
                break;
            }
        }
    }
    break;

    case NET_PACKET_INPUT: {
        if (!net->isHost)
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
        if (world->masks[id] & HAS_REPLICATION && world->replications[id].role == NETROLE_REMOTE)
            continue;

        NetEntityState *e = &snapPacket.snap.entities[count++];
        e->id             = id;
        e->compMask       = world->masks[id];
        e->prefabKind     = world->replications[id].prefabKind;
        e->pos            = world->xforms[id].drawPos;
        e->rot            = world->xforms[id].drawQuat;
        e->scale          = world->xforms[id].scale.x;

        if (world->masks[id] & HAS_BODY3)
            e->vel = world->bodies[id].vel;
        if (world->masks[id] & HAS_OWNER)
            e->ownerId = world->owners[id].ownerId;
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
    ENetPacket *packet =
        enet_packet_create(&snapPacket, sizeof(NetSnapshotPacket), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
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

    for (u32 i = 0; i < snap->eCount; i++)
    {
        NetEntityState *e      = &snap->entities[i];
        int             hostId = e->id;
        if (hostId >= MAX_ENTS)
            continue;

        int id = net->hostToLocalMap[hostId];

        if (id == -1)
        {
            if (e->prefabKind)
                id = Sol_Prefab_Factory(world, 0, e->prefabKind,
                                        (PrefabDesc){
                                            .pos     = e->pos,
                                            .rot     = e->rot,
                                            .scale   = e->scale,
                                            .netRole = NETROLE_REMOTE,
                                        });
            else
                id = Sol_Create_Ent(world, 0);

            net->hostToLocalMap[hostId] = id;
            // world->masks[id] = e->compMask;
        }

        world->xforms[id].lastPos = world->xforms[id].pos = e->pos;
        world->xforms[id].lastQuat = world->xforms[id].quat = e->rot;

        world->bodies[id].vel = e->vel;

        if (world->masks[id] & HAS_VITAL)
        {
            world->vitals[id].health = e->health;
            world->vitals[id].energy = e->energy;
        }
        if (world->masks[id] & HAS_CONTROLLER)
        {
            world->controllers[id].actionState = e->inputs;
        }
    }
}

bool Net_ShouldSend_Input(SolNet *net)
{
    return (net->state == NET_STATE_PLAYING) && !net->isHost;
}

void Net_Send_Input(SolNet *net, World *world)
{
    int id = world->playerID;
    if (!(world->masks[id] & HAS_CONTROLLER))
        return;
    CompController *controller = &world->controllers[id];

    NetInputPacket input = {
        .type       = NET_PACKET_INPUT,
        .actionMask = controller->actionState,
    };
    input.lookdir = controller->lookdir;
    input.wishdir = controller->wishdir;
    input.aimdir  = controller->aimdir;
    input.yaw     = controller->yaw;
    // input.lookdir[0] = controller->lookdir.x;
    // input.lookdir[1] = controller->lookdir.y;
    // input.lookdir[2] = controller->lookdir.z;
    // input.wishdir[0] = controller->wishdir.x;
    // input.wishdir[1] = controller->wishdir.y;
    // input.wishdir[2] = controller->wishdir.z;
    // input.aimdir[0]  = controller->aimdir.x;
    // input.aimdir[1]  = controller->aimdir.y;
    // input.aimdir[2]  = controller->aimdir.z;

    ENetPacket *packet = enet_packet_create(&input, sizeof(NetInputPacket), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(net->peer, 0, packet);
}