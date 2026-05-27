#pragma once
#include "sol/types.h"

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