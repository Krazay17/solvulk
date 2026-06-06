#pragma once
#include "sol/types.h"

typedef struct CompInteract
{
    InteractState state;
    u32           movingId;
    float         value;
    double        pressedAccum;

    Callback onClick;
    Callback onHold;
    vec2s    grabOffset, pressPos;
} CompInteract;

typedef struct
{
    int id;
    int worldId;
} InteractingEnt;

extern InteractingEnt interactingEnt;

void Sol_Interact_Update(World **world, int worldCount);
void Sol_Interact_Init(World *world);
void Sol_Interact_Set(World *world, int id, CompInteract desc);
void Sol_Interact_Add(World *world, int id);
void System_Interact_Tick(World *world, double dt, double time);

InteractState Sol_Interact_GetState(World *world, int id);
bool          Sol_Interact_GetToggle(World *world, int id);
