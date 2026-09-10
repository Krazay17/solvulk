#pragma once
#include "sol/types.h"

int Sol_Prefab_Dude(World *world, vec3s pos, float scale);
int Sol_Prefab_Wizard(World *world, vec3s pos, float scale);

int Sol_Prefab_Crosshair(World *world);
int Sol_Prefab_Button(World *world, vec3s pos, const char *text, u32 interact_flags, u32 layer, Hook func);
int Sol_Prefab_Healthbar(World *world, vec3s pos);
int Sol_Prefab_Fireball(World *world, int owner, vec3s pos, vec3s dir, float speed, float size);

