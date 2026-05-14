#pragma once
#include "sol/types.h"

typedef struct
{
    bool dealsdamage;
}CombatDesc;

void Sol_Combat_Init(World *world);
void Sol_Combat_Step(World *world, double dt, double time);

void Sol_Combat_Add(World *world, int id, CombatDesc desc);
