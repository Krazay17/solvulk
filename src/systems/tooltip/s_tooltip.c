#include "sol_core.h"

static int tooltipId = -1;

static void Tooltip_Step(World *world, double dt, double time);

void Sol_Tooltip_Init(World *world)
{
    world->tooltips = calloc(MAX_ENTS, sizeof(CompTooltip));
    WAddStep(world) = Tooltip_Step;
}

CompTooltip *Sol_Tooltip_Add(World *world, int id, TooltipKind kind)
{
    world->tooltips[id] = tooltip_configs[kind];
    world->masks[id] |= HAS_TOOLTIP;
}

static void Tooltip_Step(World *world, double dt, double time)
{
    
    // int required = HAS_TOOLTIP | HAS_INTERACT;
    // for (int i = 0; i < world->activeCount; i++)
    // {
    //     int id = world->activeEntities[i];
    //     if ((world->masks[id] & required) != required)
    //         continue;
    //     CompInteract *interact = &world->interacts[id];
    //     if (!(interact->state & INTERACT_HOVERED) || interact->state & INTERACT_MOVING)
    //         continue;

    // }
}

static void Tooltip_Draw(World *world, double dt, double time)
{
}