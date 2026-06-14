#pragma once
#include "sol/types.h"

void Sol_View_Init(World *world);

void Sol_View_Crosshair(World *world);

void Sol_View_Healthbar(World *world);

void Sol_View_Fx(World *w);
void Sol_View_Ability_Draw(World *world, double dt, double time);
void Sol_View_Healthbar_Draw(World *world, double dt, double time);
void Sol_View_Crosshair_Draw(World *world, double dt, double time);

void Sol_Render_DrawText3D(Text3DDesc desc);