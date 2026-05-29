#pragma once
#include "event/event.h"
#include "sol/types.h"

#define MAX_NET_ENTS (1 << 12)
#define MAX_NET_INTERP_DISTANCE 1.0f
#define MAX_SNAPS_BUFFERED 128
#define MAX_NET_PREDICTIONS 128
#define MAX_NET_EVENTS 24

typedef struct
{
    u8       type;
    u32      tickNumber;
    u32      worldId;
    u32      eventCount;
    SolEvent events[MAX_NET_EVENTS];
} EventSnap;

typedef struct
{
    u32     id, ownerId;
    u32     compMask;
    u32     buffMask;
    vec3s   pos, vel;
    versors rot;
    float   scale, animSeek, abilityCharge;

    u32 health, energy;

    u8 team, prefabKind;
    u8 abilityState, abilityStage;
    u8 movementState;

    u8 modelId;
    u8 animPlaying[ANIM_LAYER_COUNT];
} NetEntityState;

typedef struct
{
    u8             type;
    u32            tickNumber;
    u32            worldId;
    u32            eCount;
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
    u32       snapHead;
    WorldSnap snapShots[MAX_SNAPS_BUFFERED];

    int        predictionCount;
    Prediction predictions[MAX_NET_PREDICTIONS];

    u32  maxHostId;
    int  hostToLocalMap[MAX_ENTS];
    bool seenThisSnap[MAX_ENTS];
} WorldNet;

typedef enum
{
    NETAUTH_NONE,
    NETAUTH_REMOTE,
    NETAUTH_LOCAL,
    NETAUTH_AUTH,
} NetAuth;

typedef struct CompReplication
{
    u8  auth;
    u32 prefabKind;
} CompReplication;

void Sol_Replication_Init(World *world);
void Sol_Replication_Add(World *world, int id, NetAuth role, u8 prefabKind);

void Sol_Replication_Disconnect(World *world);

void Net_Send_Snap(World *world);
void Net_Send_Events(World *world);

void Net_Apply_Snap(World *world);
void Net_Apply_Events(World *world, EventSnap *snap);
