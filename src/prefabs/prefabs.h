#pragma once
#include "sol/types.h"

typedef struct UserData UserData;

int Sol_Prefab_Dude(World *world, vec3s pos, float scale);
int Sol_Prefab_Wizard(World *world, vec3s pos, float scale);

int Sol_Prefab_Crosshair(World *world);
int Sol_Prefab_Button(World *world, vec3s pos, const char *text, u32 interact_flags, u32 layer, Hook on_click);
int Sol_Prefab_Slider(World *world, vec3s pos, const char *text, u32 interact_flags, u32 layer, Hook on_hold);
int Sol_Prefab_Powerbar(World *world, vec3s pos, vec4s color, float dim_y);
int Sol_Prefab_Fireball(World *world, int owner, vec3s pos, vec3s dir, float speed, float size);
int Sol_Prefab_PlasmaOrb(World *world, int owner, vec3s pos, vec3s dir, float speed, float size);
int Sol_Prefab_AbilityCard(World *world, vec3s pos, SolItem *item);
int Sol_Prefab_DragonOrb(World *world, vec3s pos);
int Sol_Prefab_LightningBolt(World *world, int owner, vec3s pos, vec3s dir, float speed, float scale, Hook hook);
int Sol_Prefab_PowerVial(World *world, vec3s pos, vec4s color);
int Sol_Prefab_AbilityBar(World *world, vec3s pos, int ent_id, int ent_world_idx, const int *slots, int slot_count);
int Sol_Prefab_Inventory(World *world, vec3s pos, const ScInventory *inventory);
int Sol_Prefab_InventoryWindow(World *world, vec3s pos);
int Sol_Prefab_RulesWindow(World *world, vec3s pos);