#pragma once
#include "sol/types.h"

typedef struct
{
    bool dealsdamage;
}CombatDesc;

void Sol_Combat_Init(World *world);

void Sol_Contact_Add(World *world, int id, CombatDesc desc);
