#pragma once
#include "sol/types.h"

typedef struct World World;

void Sol_View_Init(World *world);
void Sol_View_Fx(World *w);
void Sol_Crosshair_Draw(World *world, double dt, double time);

void Sol_Nameplate_Init(World *world);