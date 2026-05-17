#pragma once
#include "sol/types.h"

#include "combat/combat_types.h"

typedef struct
{
    ImpactList impacts;
    u32        bounces;
} ContactDesc;

void Sol_Contact_Init(World *world);

void Sol_Contact_Add(World *world, int id, ContactDesc desc);