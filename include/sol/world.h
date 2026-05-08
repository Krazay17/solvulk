#pragma once
#include "components.h"

#define SYS_BIT(x) (1u << (x))

typedef struct World World;

typedef enum
{
    WORLD_SYS_TIMER,
    WORLD_SYS_XFORM,
    WORLD_SYS_PHYSX,
    WORLD_SYS_CONTROLLER,

    WORLD_SYS_MOVEMENT,
    WORLD_SYS_COMBAT,
    WORLD_SYS_INTERACT,
    WORLD_SYS_PICKUP,
    WORLD_SYS_BUFF,
    WORLD_SYS_VITAL,

    WORLD_SYS_MODEL,
    WORLD_SYS_LINE,
    WORLD_SYS_EMITTER,
    WORLD_SYS_SPHERE,

    WORLD_SYS_CAM,
    WORLD_SYS_UI,
    WORLD_SYS_COUNT,
} WorldSystem;

World *World_Create(void);
World *World_Create_Default(void);
void   World_Destroy(World *world);
void   World_System_Add(World *world, WorldSystem system);
int    Sol_World_GetEntCount(World *world);

// Add Entity
int  Sol_Create_Ent(World *world);
void Sol_Destroy_Ent(World *world, int id);

// Flags
void Sol_Flags_Add(World *world, int id, EntFlags flags);
void Sol_Flags_Remove(World *world, int id, EntFlags flags);

// Add Prefab entity
int Sol_Prefab_Player(World *world, vec3s pos, float scale);
int Sol_Prefab_Wizard(World *world, vec3s pos, float scale);
int Sol_Prefab_Floor(World *world, vec3s pos);
int Sol_Prefab_Pawn(World *world, vec3s pos, vec2s dims, float scale, SolModelId modelid, MoveConfigId moveid);
int Sol_Prefab_Button(World *world, vec3s pos, const char *text);
int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, SphereDesc desc);
