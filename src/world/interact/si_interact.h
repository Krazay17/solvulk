#pragma once
#include "s_interact.h"

#define MAX_TOOLTIP_LINES 10

typedef void (*TooltipDraw)(World *world, int id, float alpha, double dt, double time);

extern const TooltipDraw tooltip_funcs[TOOLTIPKIND_COUNT];

void Tooltip_Update(double dt, SolUserHit user_hit);
void Tooltip_Card_Draw(World *world, int id, float alpha, double dt, double time);

void Pickup_Step(World *world, double dt, double time);
