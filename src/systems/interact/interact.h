#pragma once
#include "sol/types.h"

typedef struct
{
    u8       states;
    float    value;
    Callback onClick;
    Callback onHold;
} InteractDesc;

void          Sol_Interact_Init(World *world);
void          Sol_Interact_Add(World *world, int id, InteractDesc desc);
void          System_Interact_Tick(World *world, double dt, double time);
InteractState Sol_Interact_GetState(World *world, int id);
bool          Sol_Interact_GetToggle(World *world, int id);
