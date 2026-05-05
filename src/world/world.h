#pragma once
#include "components.h"

#define MAX_SYSTEMS 64
#define MAX_ENTS (1 << 15)
#define SYS_BIT(x) (1u << (x))

typedef struct World World;
typedef void (*SystemInit)(World *);
typedef void (*SystemFunc)(World *, double, double);

typedef bool     Active;
typedef uint32_t Mask;

// fwds
typedef struct WorldPhysx  WorldPhysx;
typedef struct WorldLines  WorldLines;
typedef struct SolEmitters SolEmitters;
typedef struct SolEvents   SolEvents;

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

typedef enum
{
    SYSTEM_STEP,
    SYSTEM_TICK,
    SYSTEM_DRAW,
} SystemKind;

typedef struct
{
    SystemInit init;
    SystemFunc tick;
    SystemFunc step;
    SystemFunc draw2d;
    SystemFunc draw3d;
} SystemConfig;

typedef struct World
{
    SystemFunc stepSystems[MAX_SYSTEMS];
    SystemFunc tickSystems[MAX_SYSTEMS];
    SystemFunc draw3dSystems[MAX_SYSTEMS];
    SystemFunc draw2dSystems[MAX_SYSTEMS];

    int       activeEntities[MAX_ENTS];
    Active    actives[MAX_ENTS];
    Mask      masks[MAX_ENTS];
    CompFlags flags[MAX_ENTS];

    CompTimer      *timers;
    SolEvents      *events;
    CompXform      *xforms;
    CompBody       *bodies;
    CompMovement   *movements;
    CompModel      *models;
    CompUi         *uiElements;
    CompInteract   *interacts;
    CompSphere     *spheres;
    CompVital      *vitals;
    CompController *controllers;
    CompBuff       *buffs;
    CompAbility    *abilities;
    WorldPhysx     *spatial;
    WorldLines     *lines;
    SolEmitters    *emitters;

    bool worldActive;

    int stepCount;
    int tickCount;
    int draw2dCount;
    int draw3dCount;
    int activeCount;
    int playerID;
    u32 systemBits;
} World;

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
int Sol_Prefab_Floor(World *world, vec3s pos);
int Sol_Prefab_Pawn(World *world, vec3s pos, SolModelId modelId, float height);
int Sol_Prefab_Button(World *world, vec3s pos, const char *text);
int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, SphereDesc desc);
