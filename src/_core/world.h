#pragma once
#include "sol/types.h"

#define MAX_ENTS (1 << 15)
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
    HAS_NONE       = 0,
    HAS_XFORM      = (1 << 0),
    HAS_BODY2      = (1 << 1),
    HAS_BODY3      = (1 << 2),
    HAS_INTERACT   = (1 << 3),
    HAS_MODEL      = (1 << 4),
    HAS_UIVIEW     = (1 << 5),
    HAS_MOVEMENT   = (1 << 6),
    HAS_CONTROLLER = (1 << 7),
    HAS_ABILITY    = (1 << 8),
    HAS_BUFF       = (1 << 9),
    HAS_VITAL      = (1 << 10),
    HAS_SHAPE      = (1 << 11),
    HAS_TIMER      = (1 << 12),
    HAS_EVENT      = (1 << 13),
    HAS_AUDIO      = (1 << 14),
    HAS_PARENT     = (1 << 15),
    HAS_CONTACT    = (1 << 16),
    HAS_OWNER      = (1 << 17),
    COMPONENT_COUNT,
} CompBits;

typedef enum
{
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
    WORLD_SYS_PARENT,
    WORLD_SYS_OWNER,
    WORLD_SYS_BUFF,
    WORLD_SYS_ABILITY,
    WORLD_SYS_CONTACT,
    WORLD_SYS_COMBAT,
    WORLD_SYS_VITAL,
    WORLD_SYS_MOVEMENT,

    // Draw
    WORLD_SYS_MODEL,
    WORLD_SYS_LINE,
    WORLD_SYS_EMITTER,
    WORLD_SYS_SHAPE,
    WORLD_SYS_UI,

    WORLD_SYS_VIEW,
    WORLD_SYS_CAM,

    WORLD_SYS_COUNT,
} WorldSystem;

typedef void (*SystemInit)(World *);
typedef void (*SystemClear)(World *, int);
typedef void (*SystemFunc)(World *, double, double);

typedef bool     Active;
typedef uint32_t Mask;

typedef struct CompParent     CompParent;
typedef struct CompAudio      CompAudio;
typedef struct CompTimer      CompTimer;
typedef struct CompXform      CompXform;
typedef struct CompBody       CompBody;
typedef struct CompMovement   CompMovement;
typedef struct CompModel      CompModel;
typedef struct CompUi         CompUi;
typedef struct CompInteract   CompInteract;
typedef struct CompShape      CompShape;
typedef struct CompVital      CompVital;
typedef struct CompController CompController;
typedef struct CompBuff       CompBuff;
typedef struct CompAbility    CompAbility;
typedef struct SolEvents      SolEvents;
typedef struct WorldPhysx     WorldPhysx;
typedef struct WorldLines     WorldLines;
typedef struct SolEmitters    SolEmitters;
typedef struct CompOwner      CompOwner;
typedef struct CompContact    CompContact;

typedef struct CompFlags
{
    EFlag flags;
} CompFlags;

typedef struct World
{
    SystemFunc prestepSystems[MAX_SYSTEMS];
    SystemFunc stepSystems[MAX_SYSTEMS];
    SystemFunc poststepSystems[MAX_SYSTEMS];
    SystemFunc tickSystems[MAX_SYSTEMS];
    SystemFunc draw3dSystems[MAX_SYSTEMS];
    SystemFunc draw2dSystems[MAX_SYSTEMS];

    int       activeEntities[MAX_ENTS];
    Active    actives[MAX_ENTS];
    Mask      masks[MAX_ENTS];
    CompFlags flags[MAX_ENTS];

    CompParent     *parents;
    CompAudio      *audios;
    CompTimer      *timers;
    CompXform      *xforms;
    CompBody       *bodies;
    CompMovement   *movements;
    CompModel      *models;
    CompUi         *uiElements;
    CompInteract   *interacts;
    CompShape      *spheres;
    CompVital      *vitals;
    CompController *controllers;
    CompBuff       *buffs;
    CompAbility    *abilities;
    CompOwner      *owners;
    CompContact    *contacts;

    SolEvents   *events;
    WorldPhysx  *spatial;
    WorldLines  *lines;
    SolEmitters *emitters;
    u32 skyboxId;

    bool worldActive;

    int prestepCount;
    int stepCount;
    int poststepCount;
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

void World_Step(World *world, double dt, double time);
void World_Tick(World *world, double dt, double time);
void World_Draw3d(World *world, double dt, double time);
void World_Draw2d(World *world, double dt, double time);

int  Sol_Create_Ent(World *world);
void Sol_Destroy_Ent(World *world, int id);

void Sol_Flags_Add(World *world, int id, EFlag flags);
void Sol_Flags_Remove(World *world, int id, EFlag flags);

int Sol_World_GetEntCount(World *world);