#include <stdio.h>
#include <stdbool.h>

#define ENET_IMPLEMENTATION
#include <enet.h>

#include "Server.h"

WorldState worldState = {0};

int main()
{
    if (enet_initialize() != 0)
    {
        printf("Could not initialize ENet.\n");
        return 1;
    }

    ENetAddress address = {0};
    address.host = ENET_HOST_ANY;
    address.port = 8080; // Match this in your client!

    // Create a server host
    // 32 slots, 2 channels, no bandwidth limits
    ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

    if (server == NULL)
    {
        printf("Could not create server host. \n");
        return 1;
    }

    printf("Server started on port 8080...\n");

    ENetEvent event;

    bool running = 1;
    while (running)
    {
        /* Wait up to 1000 milliseconds for an event. (WARNING: blocking) */
        while (enet_host_service(server, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            {
                printf("A new client connected from %u.\n", event.peer->address.port);
                /* Store any relevant client information here. */
                // 1. Get the unique ID ENet assigned to this connection (e.g., 0, 1, 2...)
                int id = event.peer->incomingPeerID;

                // 2. Store that ID in the peer data so we can retrieve it later
                event.peer->data = (void *)(intptr_t)id;

                // 3. Initialize that slot in your world state
                worldState.players[id].id = id;
                worldState.players[id].active = 1;
                uint8_t welcomeData = (uint8_t)id;
                ENetPacket *packet = enet_packet_create(&welcomeData, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, 1, packet);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
                if (event.packet->dataLength == sizeof(PlayerPacket))
                {
                    int clientId = (int)(intptr_t)event.peer->data;

                    PlayerPacket *p = (PlayerPacket *)event.packet->data;
                    worldState.players[clientId].x = p->x;
                    worldState.players[clientId].y = p->y;
                    worldState.players[clientId].z = p->z;
                    worldState.players->inputState = p->inputState;
                }
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                int id = (int)(intptr_t)event.peer->data;
                printf("Client %d disconnected.\n", id);

                if (id >= 0 && id < MAX_CLIENTS)
                {
                    worldState.players[id].active = 0;
                }
                event.peer->data = NULL;
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            {

                int id = (int)(intptr_t)event.peer->data;
                printf("Client %d timed out.\n", id);

                if (id >= 0 && id < MAX_CLIENTS)
                {
                    worldState.players[id].active = 0;
                }
                event.peer->data = NULL;
                break;
            }
            case ENET_EVENT_TYPE_NONE:
                break;
            default:
                break;
            }
        }

        if (server->connectedPeers > 0)
        {
            ENetPacket *packet = enet_packet_create(&worldState,
                                                    sizeof(WorldState),
                                                    ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
            enet_host_broadcast(server, 0, packet);
            enet_host_flush(server);
        }
#if _WIN32
        Sleep(16);
#endif
    }

    enet_host_destroy(server);
    enet_deinitialize();
    return 0;
}