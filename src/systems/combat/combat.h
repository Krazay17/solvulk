#pragma once
#include "sol/types.h"

#include "combat_types.h"

typedef struct
{
    SolHit hit;
    u32    bounces;
} ContactDesc;

void Sol_Contact_Init(World *world);
void Sol_Combat_Init(World *world);

void Sol_Contact_Add(World *world, int id, ContactDesc desc);
