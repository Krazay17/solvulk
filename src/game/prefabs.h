#pragma once
#include "sol/types.h"

typedef struct
{
    vec3s   pos, vel;
    versors rot;
    float   scale;
    u8      authority;
} EntDesc;

int Sol_Prefab_Factory(World *world, u32 id, u32 kind, EntDesc desc);
int Sol_Prefab_Player(World *world, u32 id, vec3s pos, float scale);
int Sol_Prefab_Wizard(World *world, u32 id, vec3s pos, float scale);
int Sol_Prefab_Fireball(World *world, u32 id, vec3s pos, float scale);
int Sol_Prefab_Healthbar(World *world, vec3s pos, World *otherWorld, u32 otherId);
int Sol_Prefab_Bullet(World *world, u32 id, vec3s pos, float scale);
int Sol_Prefab_Button(World *world, vec3s pos, const char *text);
int Sol_Prefab_Clouds(World *world, vec3s pos);
int Sol_Prefab_AbilityCard(World *world, vec3s pos, AbilityState ability, u32 rarity);
int Sol_Prefab_AbilitySlot(World *world, vec3s pos, u32 slot, char *label);
int Sol_Prefab_Zorgon(World *world, u32 id, vec3s pos, float scale);