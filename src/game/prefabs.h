#pragma once
#include "sol/types.h"

int Sol_Prefab_Dude(World *world, vec3s pos, float scale);
int Sol_Prefab_Crosshair(World *world);
int Sol_Prefab_Button(World *world, vec3s pos, const char *text, u32 interact_mask);
int Sol_Prefab_Healthbar(World *world, vec3s pos);
