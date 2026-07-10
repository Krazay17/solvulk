#pragma once
#include "types.h"

typedef struct World World;

typedef struct CompInteract
{
    InteractState state;
    u32           movingId;
    float         value;
    double        pressedAccum;

    SolCallback onClick;
    SolCallback onHold;
    vec2s    grabOffset, pressPos;
} CompInteract;

typedef enum
{
    TOOLTIPKIND_CARD,
    TOOLTIPKIND_PLAYER_INTERACT,
    TOOLTIPKIND_COUNT,
} TooltipKind;

typedef struct CompTooltip
{
    TooltipKind kind;
} CompTooltip;


CompTooltip *Sol_Tooltip_Add(World *world, int id, TooltipKind kind);
void         Sol_Tooltip_Update(double dt);
void         Sol_Tooltip_Draw();

void          Sol_Interact_Init(World *world);
void          Sol_Interact_Set(World *world, int id, CompInteract desc);
CompInteract *Sol_Interact_Add(World *world, int id);
InteractState Sol_Interact_GetState(World *world, int id);
bool          Sol_Interact_GetToggle(World *world, int id);

void Sol_Pickup_Init(World *world);
void Sol_Pickup_Step(World *world, double dt, double time);