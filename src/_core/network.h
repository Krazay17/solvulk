#pragma once
#include "sol/base.h"

#include "enet.h"

#define MAX_NET_CLIENTS 12
#define MAX_SNAPS_BUFFERED 128
#define MAX_NET_ENTS 120

typedef struct World World;

typedef enum
{
    NET_PACKET_HELLO = 1, // client → server: I'm joining
    NET_PACKET_WELCOME,   // server → client: here's your assignment
    NET_PACKET_REJECT,    // server → client: can't join (server full, version mismatch, etc.)

    NET_PACKET_SNAPSHOT, // server → client: world state
    NET_PACKET_INPUT,    // client → server: player input
} NetPacketType;

typedef struct
{
    u32     id, compMask;
    vec3s   pos, scale;
    versors rot;
    u32     health, energy;
    u32     inputs;
} NetEntityState;

typedef struct
{
    u32            tickNumber;
    u32            worldId;
    u32            eCount;
    NetEntityState entities[MAX_NET_ENTS];
} WorldSnap;

typedef struct WorldNet
{
    u32       snapHead;
    WorldSnap snapShots[MAX_SNAPS_BUFFERED];
} WorldNet;

typedef struct
{
    u8        type;
    u32       worldId;
    WorldSnap snap;
} NetSnapshotPacket;

typedef struct
{
    u8 type; // NET_PACKET_HELLO

    u32   worldId;         // which world the client wants to join
    vec3s startPos;        // where the client wants to spawn
    char  name[32];        // player display name (optional)
    u32   protocolVersion; // for compatibility check
} NetHelloPacket;

typedef struct
{
    u8 type; // NET_PACKET_WELCOME

    u32       playerId;    // the entity ID the client is assigned
    u32       worldId;     // confirmed world ID (may differ from requested)
    u32       currentTick; // server's current tick for sync
    WorldSnap snapShot;
} NetWelcomePacket;

typedef enum
{
    REJECT_SERVER_FULL,
    REJECT_VERSION_MISMATCH,
    REJECT_WORLD_NOT_FOUND,
    REJECT_BANNED,
} RejectReason;

typedef struct
{
    u8   type;         // NET_PACKET_REJECT
    u32  reason;       // RejectReason
    char message[128]; // optional human-readable
} NetRejectPacket;

typedef enum
{
    PLAYER_STATE_AWAITING_HELLO, // ENet connected, no HELLO yet
    PLAYER_STATE_PLAYING,        // sent WELCOME, full player
} PlayerState;

typedef struct
{
    PlayerState       state;
    u32               currentWorldId;
    u32               entityId;
    struct _ENetPeer *enetPeerHandle;
} NetPlayer;

typedef enum
{
    NET_STATE_DISCONNECTED,
    NET_STATE_CONNECTING,       // ENet handshake in progress
    NET_STATE_AWAITING_WELCOME, // ENet connected, sent HELLO, waiting
    NET_STATE_PLAYING,          // got WELCOME, in game
    NET_STATE_DISCONNECTING,
} NetState;

typedef struct
{
    bool      isConnected, isHost;
    u32       connectedPlayerCount;
    NetPlayer players[MAX_NET_CLIENTS];

    NetState state;
    u32      localEntityId;  // client's view of own entity
    u32      serverEntityId; // server's assigned ID (after WELCOME)

    struct _ENetHost *host;
    struct _ENetPeer *peer;
} SolNet;

void Net_Init(SolNet *net, bool host, const char *ip, u16 port);
void Net_DeInit(SolNet *net);

bool Net_ShouldSend(SolNet *net);
void Net_Poll(SolNet *net);
void Net_Recv_Packet(SolNet *net, ENetEvent *event);

int  Net_World_Init(World *world);
void Net_Send_Snap(SolNet *net, World *world);
void Net_Apply_Snap(World *world);