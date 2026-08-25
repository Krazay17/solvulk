#pragma once
#include "types.h"

#define MAX_TOOLTIP_ALPHA 0.9f

typedef struct CompInteract
{
    double        hover_start_time;
    double        unhover_start_time;
    double        press_start_time;
    InteractState state;
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

void Sol_Interact_Init(World *world);

void          Sol_Tooltip_Draw(SolUserHit user_hit, float alpha, double dt, double time);
CompTooltip  *Sol_Tooltip_Add(World *world, int id, TooltipKind kind);
CompInteract *Sol_Interact_Add(World *world, int id);
bool          Sol_Interact_Has(World *world, int id);
CompInteract *Sol_Interact_Get(World *world, int id);

void          Sol_Interact_Set(World *world, int id, CompInteract desc);
InteractState Sol_Interact_GetState(World *world, int id);
bool          Sol_Interact_GetToggle(World *world, int id);
int           Sol_Interact_GetTopmost(World *world);
void          Sol_Interact_AddState(World *world, int id, InteractState state);
void          Sol_Interact_RemState(World *world, int id, InteractState state);
float         Sol_Interact_GetHoverWeight(const CompInteract *interact, double currentTime, float duration);

void Sol_Interact_DragEntityTo(World *world, int id, vec3s targetPos);
void Sol_Interact_EndDrag(World *world, int id);
