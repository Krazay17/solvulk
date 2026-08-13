#pragma once
#include "types.h"

typedef struct World World;

typedef struct CompInteract
{
    InteractState state;
    float         value;
    double        pressedAccum;

    SolCallback onClick;
    SolCallback onHold;

    vec3s offset, targetPos;
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
void         Sol_Tooltip_Update(double dt, SolUserHit user_hit);
void         Sol_Tooltip_Draw(double dt, SolUserHit user_hit);

void          Sol_Interact_Init(World *world);
void          Sol_Interact_Set(World *world, int id, CompInteract desc);
CompInteract *Sol_Interact_Add(World *world, int id);
InteractState Sol_Interact_GetState(World *world, int id);
bool          Sol_Interact_GetToggle(World *world, int id);
int           Sol_Interact_GetTopmost(World *world);
void          Sol_Interact_AddState(World *world, int id, InteractState state);
void          Sol_Interact_ClearState(World *world, int id, InteractState state);

void Sol_Interact_DragEntityTo(World *world, int id, vec3s targetPos);
void Sol_Interact_EndDrag(World *world, int id);

void Sol_Pickup_Init(World *world);
void Sol_Pickup_Step(World *world, double dt, double time);