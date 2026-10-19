/*
 * File: s_ribbon.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-19
 * 
*/
#pragma once
#include "components.h"

extern const void (*ribbon_draw_kind[RIBBONKIND_COUNT])(World *world, double dt, Ribbon *r);

ScRibbon *Sol_Ribbon_AddKind(World *world, int id, RibbonKind kind);
Ribbon *Sol_Ribbon_Next(World *world);
void Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s posA, vec3s posB);
void Sol_Ribbon_SpawnE(World *world, RibbonKind kind, u32 entA, u32 entB);
int Sol_Ribbon_Addpoint(Ribbon *r, vec3s pos, vec3s side);
void Sol_Ribbon_Addpoint_Jitter(Ribbon *r, vec3s pos, vec3s side, float jitter_mag);
