#pragma once
#include "sol/types.h"

typedef struct World World;

int Sol_Prefab_Player(World *world, vec3s pos, float scale);
int Sol_Prefab_Wizard(World *world, vec3s pos, float scale);
int Sol_Prefab_Floor(World *world, vec3s pos);
int Sol_Prefab_Button(World *world, vec3s pos, const char *text);
int Sol_Prefab_Pawn(World *world, vec3s pos, vec2s dims, float scale, SolModelId modelid, MoveConfigId moveid);
int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, int ownerId, ShapeDesc shape, ContactDesc contact);
int Sol_Prefab_Fireball(World *world, int ownerId, vec3s pos, vec3s vel, float radius, u32 damage);
int Sol_Prefab_Clouds(World *world, vec3s pos);