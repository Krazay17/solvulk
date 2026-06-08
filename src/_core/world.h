#pragma once
#include "sol/types.h"

#define MAX_SYSTEMS 64
#define SYS_BIT(x) (1u << (x))

#define WAdd2d(w) w->draw2dSystems[w->draw2dCount++]
#define WAdd3d(w) w->draw3dSystems[w->draw3dCount++]
#define WAddPrestep(w) w->prestepSystems[w->prestepCount++]
#define WAddStep(w) w->stepSystems[w->stepCount++]
#define WAddPoststep(w) w->poststepSystems[w->poststepCount++]
#define WAddTick(w) w->tickSystems[w->tickCount++]

typedef enum
{
    WORLDKIND_GAME,
    WORLDKIND_MENU,
} WorldKind;

typedef enum
{
    HAS_NONE         = 0,
    HAS_ACTIVE       = (1 << 0),
    HAS_BODY2        = (1 << 2),
    HAS_BODY3        = (1 << 3),
    HAS_INTERACT     = (1 << 4),
    HAS_MODEL        = (1 << 5),
    HAS_MOVEMENT     = (1 << 7),
    HAS_CONTROLLER   = (1 << 8),
    HAS_ABILITY      = (1 << 9),
    HAS_BUFF         = (1 << 10),
    HAS_VITAL        = (1 << 11),
    HAS_SHAPE        = (1 << 12),
    HAS_TIMER        = (1 << 13),
    HAS_EVENT        = (1 << 14),
    HAS_AUDIO        = (1 << 15),
    HAS_PARENT       = (1 << 16),
    HAS_CONTACT      = (1 << 17),
    HAS_OWNER        = (1 << 18),
    HAS_AICONTROLLER = (1 << 19),
    HAS_COMBAT       = (1 << 20),
    HAS_REPLICATION  = (1 << 21),
    HAS_EMITTER      = (1 << 22),
    HAS_VIEW2D       = (1 << 23),
    HAS_TRACKER      = (1 << 24),
    HAS_PROJECTILE   = (1 << 25),
    HAS_ITEM         = (1 << 26),
    HAS_TOOLTIP      = (1 << 27),
    COMPONENT_COUNT,
} CompBits;

typedef enum
{
    // Replication pumps events for systems
    WORLD_SYS_REPLICATION,
    
    // Misc
    WORLD_SYS_XFORM,
    WORLD_SYS_EVENT,

    // Tick
    WORLD_SYS_CONTROLLER,
    WORLD_SYS_INTERACT,

    // Step
    WORLD_SYS_TIMER,
    WORLD_SYS_PICKUP,
    WORLD_SYS_PHYSX,
    WORLD_SYS_BODY2,
    WORLD_SYS_OWNER,
    WORLD_SYS_PARENT,

    // Event flow
    WORLD_SYS_BUFF,
    WORLD_SYS_ABILITY,
    WORLD_SYS_ITEM,
    WORLD_SYS_PROJECTILE,
    WORLD_SYS_AICONTROLLER,

    WORLD_SYS_COMBAT,

    WORLD_SYS_VITAL,
    WORLD_SYS_MOVEMENT,

    // Draw
    WORLD_SYS_MODEL,
    WORLD_SYS_LINE,
    WORLD_SYS_EMITTER,
    WORLD_SYS_SHAPE,
    WORLD_SYS_VIEW2D,
    WORLD_SYS_UI,

    WORLD_SYS_VIEW,

    WORLD_SYS_COUNT,
} WorldSystem;

typedef void (*SystemInit)(World *);
typedef void (*SystemClear)(World *, int);
typedef void (*SystemFunc)(World *, double, double);

typedef uint64_t Mask;

typedef struct CompReplication  CompReplication;
typedef struct CompAiController CompAiController;
typedef struct CompParent       CompParent;
typedef struct CompAudio        CompAudio;
typedef struct CompTimer        CompTimer;
typedef struct CompXform        CompXform;
typedef struct CompBody         CompBody;
typedef struct CompMovement     CompMovement;
typedef struct CompModel        CompModel;
typedef struct CompInteract     CompInteract;
typedef struct CompShape        CompShape;
typedef struct CompVital        CompVital;
typedef struct CompController   CompController;
typedef struct CompBuff         CompBuff;
typedef struct CompAbility      CompAbility;
typedef struct CompOwner        CompOwner;
typedef struct CompContact      CompContact;
typedef struct CompCombat       CompCombat;
typedef struct CompEmitter      CompEmitter;
typedef struct CompBody2d       CompBody2d;
typedef struct CompView2d       CompView2d;
typedef struct CompProjectile   CompProjectile;
typedef struct CompItem         CompItem;
typedef struct CompTooltip      CompTooltip;

typedef struct SolEvents   SolEvents;
typedef struct SolEmitters SolEmitters;
typedef struct WorldPhysx  WorldPhysx;
typedef struct WorldLines  WorldLines;
typedef struct SolCamera   SolCamera;
typedef struct WorldNet    WorldNet;

typedef struct CompFlags
{
    EFlag flags;
} CompFlags;
typedef struct CompTracker
{
    World *world;
    u32    entId;
} CompTracker;

typedef struct World
{
    SystemFunc prestepSystems[MAX_SYSTEMS];
    SystemFunc stepSystems[MAX_SYSTEMS];
    SystemFunc poststepSystems[MAX_SYSTEMS];
    SystemFunc tickSystems[MAX_SYSTEMS];
    SystemFunc draw3dSystems[MAX_SYSTEMS];
    SystemFunc draw2dSystems[MAX_SYSTEMS];

    int         activeEntities[MAX_ENTS];
    Mask        masks[MAX_ENTS];
    CompFlags   flags[MAX_ENTS];
    CompTracker trackers[MAX_ENTS];

    CompReplication  *replications;
    CompParent       *parents;
    CompAudio        *audios;
    CompTimer        *timers;
    CompXform        *xforms;
    CompBody         *bodies;
    CompMovement     *movements;
    CompModel        *models;
    CompInteract     *interacts;
    CompShape        *spheres;
    CompVital        *vitals;
    CompController   *controllers;
    CompBuff         *buffs;
    CompAbility      *abilities;
    CompOwner        *owners;
    CompContact      *contacts;
    CompAiController *aicontrollers;
    CompCombat       *combats;
    CompEmitter      *compEmitters;
    CompBody2d       *body2d;
    CompView2d       *view2d;
    CompProjectile   *projectiles;
    CompItem         *items;
    CompTooltip      *tooltips;

    u32 skyboxId;

    SolEvents   *events;
    WorldPhysx  *spatial;
    WorldLines  *lines;
    SolEmitters *emitters;
    WorldNet    *worldNet;

    bool doesSimulate;
    bool doesRender;

    int prestepCount;
    int stepCount;
    int poststepCount;
    int tickCount;
    int draw2dCount;
    int draw3dCount;
    int activeCount;
    int playerID;
    u32 systemBits;

    WorldKind kind;

    bool doesReplicate;
    u32  worldId;
    u32  currentTick;
} World;

World *World_Create(WorldKind kind);
World *World_Create_Default(WorldKind kind);
World *Sol_GetWorldById(u32 id);
void   World_SetDoesrender(World *world, bool doesRender);

void World_Destroy(World *world);
void World_System_Add(World *world, WorldSystem system);

void World_Step(World *world, double dt, double time);
void World_Tick(World *world, double dt, double time);
void World_Draw3d(World *world, double dt, double time);
void World_Draw2d(World *world, double dt, double time);

int  Sol_Create_Ent(World *world, u32 id);
void Sol_Destroy_Ent(World *world, int id);

void Sol_Flags_Add(World *world, int id, EFlag flags);
void Sol_Flags_Remove(World *world, int id, EFlag flags);

int  Sol_World_GetEntCount(World *world);
void Sol_World_SetActive(World *world);
void Sol_World_SetReplicates(World *world, bool active);
void Sol_World_SetTracker(World *world, int id, World *otherWorld, int otherId);

void Worlds_Tick(World **worlds, int count, double dt, double time);
void Worlds_Step(World **worlds, int count, double dt, double time);
void Worlds_Draw3d(World **worlds, int count, double dt, double time);
void Worlds_Draw2d(World **worlds, int count, double dt, double time);