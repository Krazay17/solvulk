#pragma once
#include "sol/types.h"

#include "enet.h"

#define MAX_NET_CLIENTS 12
#define MAX_SNAPS_BUFFERED 128
#define MAX_NET_ENTS (1 << 12)
#define MAX_NET_EVENTS 24
#define MAX_NET_PREDICTIONS 24

typedef struct World World;

typedef enum
{
    NET_PACKET_HELLO = 1, // client → server: I'm joining
    NET_PACKET_WELCOME,   // server → client: here's your assignment
    NET_PACKET_REJECT,    // server → client: can't join (server full, version mismatch, etc.)

    NET_PACKET_SNAPSHOT, // server → client: world state
    NET_PACKET_INPUT,    // client → server: player input
    NET_PACKET_EVENT,
} NetPacketType;

typedef struct
{
    u32     id, compMask, prefabKind;
    u32     ownerId, team;
    vec3s   pos, vel;
    versors rot;
    u32     health, energy;
    u32     inputs;
    float   scale;
    bool    active;
} NetEntityState;

typedef struct
{
    u8 type;
    u32 tickNumber;
    u32 worldId;
    u32 eCount;
    NetEntityState entities[MAX_NET_ENTS];
} WorldSnap;

typedef struct
{
    int  localEntId;
    bool reconciled;
    u32  prefabKind;
    u32  tickSpawned;
} Prediction;
typedef struct WorldNet
{
    u32        snapHead;
    WorldSnap  snapShots[MAX_SNAPS_BUFFERED];
    int        hostToLocalMap[MAX_ENTS];
    u32        maxHostId;
    bool       seenThisSnap[MAX_ENTS];
    Prediction predictions[MAX_NET_PREDICTIONS];
    int        predictionCount;
} WorldNet;

// Packets -----------------
// typedef struct
// {
//     u8       type;
//     SolEvent event[MAX_NET_EVENTS];
//     u32      count;
// } NetEventPacket;

typedef struct
{
    u8    type;
    bool  isStrafing;
    u32   actionMask, currentTick;
    vec3s lookdir, wishdir, aimdir;
    float yaw;
} NetInputPacket;

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

    u32 playerId;    // the entity ID the client is assigned
    u32 worldId;     // confirmed world ID (may differ from requested)
    u32 currentTick; // server's current tick for sync
} NetWelcomePacket;
// ########################
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
    NETROLE_NONE,
    NETROLE_HOST,
    NETROLE_CLIENT,
} NetRole;

typedef enum
{
    NETSTATUS_DISCONNECTED,
    NETSTATUS_CONNECTING,
    NETSTATUS_CONNECTED,
} NetStatus;

typedef struct
{
    NetRole   role;
    NetStatus status;
    u32       connectedPlayerCount;
    NetPlayer players[MAX_NET_CLIENTS];

    u32 localEntityId;  // client's view of own entity
    u32 serverEntityId; // server's assigned ID (after WELCOME)

    struct _ENetHost *host;
    struct _ENetPeer *peer;
} SolNet;

void Net_Init(SolNet *net, bool host, const char *ip, u16 port);
void Net_DeInit(SolNet *net);

bool Net_ShouldSend_Input(SolNet *net);
bool Net_ShouldSend_Snap(SolNet *net);
void Net_Poll(SolNet *net);
void Net_Recv_Packet(SolNet *net, ENetEvent *event);

int  Net_World_Init(World *world);
void Net_Apply_Snap(World *world);

void Net_Send_Input(SolNet *net, World *world);
void Net_Send_Snap(SolNet *net, World *world);

static inline bool Net_IsActive(SolNet *net)
{
    return net->role != NETROLE_NONE && net->host != NULL;
}

static inline bool Net_IsPlaying(SolNet *net)
{
    return Net_IsActive(net) && net->status == NETSTATUS_CONNECTED;
}

static inline bool Net_IsHost(SolNet *net)
{
    return net->role == NETROLE_HOST;
}

static inline bool Net_IsClient(SolNet *net)
{
    return net->role == NETROLE_CLIENT;
}