#pragma once
#include "sol/types.h"

typedef struct World World;
typedef enum
{
    PREFABKIND_PLAYER = 1,
    PREFABKIND_WIZARD,
    PREFABKIND_FIREBALL,
} PrefabKind;
typedef struct
{
    vec3s   pos, vel;
    versors rot;
    float   scale;
    u8      netRole;
} PrefabDesc;

int Sol_Prefab_Factory(World *world, u32 id, u32 kind, PrefabDesc desc);
int Sol_Prefab_Player(World *world, u32 id, vec3s pos, float scale);
int Sol_Prefab_Wizard(World *world, u32 id, vec3s pos, float scale);
int Sol_Prefab_Fireball(World *world, u32 id, vec3s pos, float scale);

int Sol_Prefab_Floor(World *world, vec3s pos);
int Sol_Prefab_Button(World *world, vec3s pos, const char *text);
int Sol_Prefab_Pawn(World *world, vec3s pos, vec2s dims, float scale, SolModelId modelid, MoveConfigId moveid);
int Sol_Prefab_Clouds(World *world, vec3s pos);