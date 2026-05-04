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
    WORLD_SYS_PHYSX,

    WORLD_SYS_MOVEMENT,
    WORLD_SYS_INTERACT,
    WORLD_SYS_PICKUP,
    WORLD_SYS_COMBAT,
    WORLD_SYS_BUFF,
    WORLD_SYS_VITAL,

    WORLD_SYS_CONTROLLER_LOCAL,
    WORLD_SYS_CONTROLLER_AI,

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
    SystemFunc tickSystems[MAX_SYSTEMS];
    SystemFunc stepSystems[MAX_SYSTEMS];
    SystemFunc draw2dSystems[MAX_SYSTEMS];
    SystemFunc draw3dSystems[MAX_SYSTEMS];

    int          activeEntities[MAX_ENTS];
    Active       actives[MAX_ENTS];
    Mask         masks[MAX_ENTS];
    CompFlags    flags[MAX_ENTS];

    CompTimer    timers[MAX_ENTS];
    CompInteract interacts[MAX_ENTS];
    CompUiView   uiElements[MAX_ENTS];

    CompSphere     *spheres;
    CompVital      *vitals;
    CompXform      *xforms;
    CompBody       *bodies;
    CompMovement   *movements;
    CompController *controllers;
    CompBuff       *buffs;
    CompModel      *models;
    CompAbility    *abilities;

    bool worldActive;

    int stepCount;
    int tickCount;
    int draw3dCount;
    int draw2dCount;
    int activeCount;
    int playerID;
    u32 systemBits;

    WorldPhysx  *spatial;
    WorldLines  *lines;
    SolEmitters *emitters;
    SolEvents   *events;
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
int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, CompSphere sphere);
int Sol_Prefab_Boxman(World *world, vec3s pos);

// Add thing to world
void Sol_World_Line_Add(World *world, vec3s a, vec3s b, vec3s color, vec3s bColor, float ttl);


void         Sol_System_Camera_Tick(World *world, double dt, double time);
SolRayResult Sol_ScreenRaycast(World *world, float screenX, float screenY, SolRay ray);

void Lines_Init(World *world);
void Sol_System_Line_Tick(World *world, double dt, double time);
void Sol_Draw_Line(SolLine *lines, int count);

CompUiView *Sol_UiView_Add(World *world, int id);
CompTimer  *Sol_Timer_Add(World *world, int id, CompTimer init);
void        Vital_Step(World *world, double dt, double time);
void        Pickup_Step(World *world, double dt, double time);

void Sol_Timer_Tick(World *world, double dt, double time);

void Sol_Line_Draw(World *world, double dt, double time);
void Sol_UiView_Draw(World *world, double dt, double time);

void Sol_Draw_Rectangle(SolRect rect, vec4s color, float thickness);
void Sol_Cam3d_Tick(World *world, double dt, double time, float alpha);
void Sol_Vital_Draw(World *world, double dt, double time);
void Sol_Crosshair_Draw(World *world, double dt, double time);

void Emitter_Init(World *world);
void Emitter_Add(World *world, Emitter e);
void Emitter_Step(World *world, double dt, double time);
void Emitter_Tick(World *world, double dt, double time);
void Emitter_Draw(World *world, double dt, double time);

void Sol_Event_Add(World *world, EventDesc desc);
void Event_Init(World *world);
void Event_Clear(World *world);