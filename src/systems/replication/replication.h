#pragma once
#include "sol/types.h"

typedef enum
{
    NETROLE_LOCAL,
    NETROLE_AUTH,
    NETROLE_REMOTE,
} NetRole;

typedef struct CompReplication
{
    u8  role;
    u32 prefabKind;
} CompReplication;

void Sol_Replication_Init(World *world);
void Sol_Replication_Add(World *world, int id, NetRole role, u8 prefabKind);