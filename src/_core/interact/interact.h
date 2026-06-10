#pragma once
#include "sol/types.h"

#define MAX_TOOLTIP_LINES 8

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
    int    id;
    int    movingId;
    World *world;
} InteractingEnt;

extern InteractingEnt interactingEnt;

void          Sol_Interact_Update(World **world, int worldCount);
void          Sol_Interact_Init(World *world);
void          Sol_Interact_Set(World *world, int id, CompInteract desc);
CompInteract *Sol_Interact_Add(World *world, int id);
InteractState Sol_Interact_GetState(World *world, int id);
bool          Sol_Interact_GetToggle(World *world, int id);

CompTooltip *Sol_Tooltip_Add(World *world, int id, TooltipKind kind);
void         Sol_Tooltip_Update(double dt);
void         Sol_Tooltip_Draw();
