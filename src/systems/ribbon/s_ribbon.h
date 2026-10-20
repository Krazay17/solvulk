/*
 * File: s_ribbon.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-19
 * 
*/
#pragma once
#include "components.h"

extern void (*const ribbon_draw_kind[RIBBONKIND_COUNT])(World *world, double dt, Ribbon *r);

Ribbon Ribbon_Make(RibbonKind kind);

Ribbon *Sol_Ribbon_AddKind(World *world, int id, RibbonKind kind);
Ribbon *Sol_Ribbon_Next(World *world, RibbonKind kind);
Ribbon *Sol_Ribbon_Find(ScRibbon *sc, RibbonKind kind);
void Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s posA, vec3s posB);
void Sol_Ribbon_SpawnE(World *world, RibbonKind kind, u32 entA, u32 entB);
bool Sol_Ribbon_Due(Ribbon *r, float dt, float interval);

int Sol_Ribbon_Addpoint(Ribbon *r, vec3s pos, vec3s side);
void Sol_Ribbon_Addpoint_Jitter(Ribbon *r, vec3s pos, vec3s side, float jitter_mag);
