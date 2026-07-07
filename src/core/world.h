#pragma once
#include "types.h"

typedef struct World World;

#define MAX_ENTS (1 << 14)
#define MAX_SYSTEMS 64

#define WAdd2d(w) w->draw2dSystems[w->draw2dCount++]
#define WAdd3d(w) w->draw3dSystems[w->draw3dCount++]
#define WAddPrestep(w) w->prestepSystems[w->prestepCount++]
#define WAddStep(w) w->stepSystems[w->stepCount++]
#define WAddPoststep(w) w->poststepSystems[w->poststepCount++]
#define WAddTick(w) w->tickSystems[w->tickCount++]
#define WAddDeinit(w) w->deinitSystems[w->deinitCount++]
#define WGetComp(w, id, comp, type) (&((type *)w->components[comp])[id])
#define WAddComp(w, id, comp) (w->masks[id] |= BITC(comp))
#define WHas(w, id, mask) ((w->masks[id] & (mask)) == (mask))

#define SOL_SYSTEM_LIST(X)                                                                                             \
    X(WORLD_SYS_REPLICATION, Sol_Replication_Init)                                                                     \
    X(WORLD_SYS_XFORM, Sol_Xform_Init)                                                                                 \
    X(WORLD_SYS_EVENT, Sol_Event_Init)                                                                                 \
    X(WORLD_SYS_CONTROLLER, Sol_Controller_Init)                                                                       \
    X(WORLD_SYS_INTERACT, Sol_Interact_Init)                                                                           \
    X(WORLD_SYS_TIMER, Sol_Timer_Init)                                                                                 \
    X(WORLD_SYS_PICKUP, Sol_Pickup_Init)                                                                               \
    X(WORLD_SYS_OWNER, Sol_Owner_Init)                                                                                 \
    X(WORLD_SYS_PARENT, Sol_Parent_Init)                                                                               \
    X(WORLD_SYS_BUFF, Sol_Buff_Init)                                                                                   \
    X(WORLD_SYS_ABILITY, Sol_Ability_Init)                                                                             \
    X(WORLD_SYS_CHAINHIT, Sol_Chainhit_Init)                                                                           \
    X(WORLD_SYS_ITEM, Sol_Item_Init)                                                                                   \
    X(WORLD_SYS_PHYSX, Sol_Physx_Init)                                                                                 \
    X(WORLD_SYS_BODY2, Sol_Body2d_Init)                                                                                \
    X(WORLD_SYS_PROJECTILE, Sol_Projectile_Init)                                                                       \
    X(WORLD_SYS_COMBAT, Sol_Combat_Init)                                                                               \
    X(WORLD_SYS_VITAL, Sol_Vital_Init)                                                                                 \
    X(WORLD_SYS_AICONTROLLER, Sol_Ai_Init)                                                                             \
    X(WORLD_SYS_MOVEMENT, Sol_Movement_Init)                                                                           \
    X(WORLD_SYS_AUDIO, Sol_World_Audio_Init)                                                                           \
    X(WORLD_SYS_MODEL, Sol_Model_Init)                                                                                 \
    X(WORLD_SYS_LINE, Sol_Line_Init)                                                                                   \
    X(WORLD_SYS_EMITTER, Sol_Emitter_Init)                                                                             \
    X(WORLD_SYS_RIBBON, Sol_Ribbon_Init)                                                                               \
    X(WORLD_SYS_SHAPE, Sol_Shape_Init)                                                                                 \
    X(WORLD_SYS_SCORE, Sol_Score_Init)                                                                                 \
    X(WORLD_SYS_VIEW2D, Sol_View2d_Init)                                                                               \
    X(WORLD_SYS_VIEW, Sol_View_Init)

typedef enum
{
#define AS_ENUM(enum_name, init_func) enum_name,
    SOL_SYSTEM_LIST(AS_ENUM)
#undef AS_ENUM
    WORLD_SYS_COUNT
} WorldSystem;

#define AS_FORWARD_DEC(enum_name, init_func) void init_func(World *world);
SOL_SYSTEM_LIST(AS_FORWARD_DEC)
#undef AS_FORWARD_DEC

typedef enum
{
    HAS_NONE,
    HAS_ACTIVE,
    HAS_XFORM,
    HAS_BODY2,
    HAS_BODY3,
    HAS_INTERACT,
    HAS_MODEL,
    HAS_MOVEMENT,
    HAS_CONTROLLER,
    HAS_ABILITY,
    HAS_BUFF,
    HAS_VITAL,
    HAS_SHAPE,
    HAS_TIMER,
    HAS_EVENT,
    HAS_AUDIO,
    HAS_PARENT,
    HAS_CONTACT,
    HAS_OWNER,
    HAS_AICONTROLLER,
    HAS_COMBAT,
    HAS_REPLICATION,
    HAS_EMITTER,
    HAS_VIEW2D,
    HAS_TRACKER,
    HAS_PROJECTILE,
    HAS_ITEM,
    HAS_ABILITYSLOT,
    HAS_TOOLTIP,
    HAS_RIBBON,
    HAS_CHAINHIT,
    HAS_INVENTORY,
    COMPONENT_COUNT,
} SolComponents;

// typedef enum
// {
//     // Replication pumps events for systems
//     WORLD_SYS_REPLICATION,

//     // Misc
//     WORLD_SYS_XFORM,
//     WORLD_SYS_EVENT,

//     // Tick
//     WORLD_SYS_CONTROLLER,
//     WORLD_SYS_INTERACT,

//     // Step
//     WORLD_SYS_TIMER,
//     WORLD_SYS_PICKUP,
//     WORLD_SYS_OWNER,
//     WORLD_SYS_PARENT,

//     // Event flow
//     WORLD_SYS_BUFF,
//     WORLD_SYS_ABILITY,
//     WORLD_SYS_CHAINHIT,
//     WORLD_SYS_ITEM,

//     WORLD_SYS_PHYSX,
//     WORLD_SYS_BODY2,
//     WORLD_SYS_PROJECTILE,

//     WORLD_SYS_COMBAT,
//     WORLD_SYS_VITAL,
//     WORLD_SYS_AICONTROLLER,
//     WORLD_SYS_MOVEMENT,

//     WORLD_SYS_AUDIO,

//     // Draw
//     WORLD_SYS_MODEL,
//     WORLD_SYS_LINE,
//     WORLD_SYS_EMITTER,
//     WORLD_SYS_RIBBON,
//     WORLD_SYS_SHAPE,
//     WORLD_SYS_VIEW2D,

//     WORLD_SYS_VIEW,

//     WORLD_SYS_COUNT,
// } WorldSystem;

typedef enum
{
    WORLDKIND_GAME,
    WORLDKIND_MENU,
} WorldKind;

typedef void (*SystemFunc)(World *);
typedef void (*SystemUpdate)(World *, double, double);

typedef uint64_t Mask;

typedef struct CompAudio       CompAudio;
typedef struct CompReplication CompReplication;
typedef struct CompAi          CompAi;
typedef struct CompParent      CompParent;
typedef struct CompTimer       CompTimer;
typedef struct CompXform       CompXform;
typedef struct CompBody        CompBody;
typedef struct CompMovement    CompMovement;
typedef struct CompModel       CompModel;
typedef struct CompInteract    CompInteract;
typedef struct CompShape       CompShape;
typedef struct CompVital       CompVital;
typedef struct CompController  CompController;
typedef struct CompBuff        CompBuff;
typedef struct CompAbility     CompAbility;
typedef struct CompOwner       CompOwner;
typedef struct CompContact     CompContact;
typedef struct CompCombat      CompCombat;
typedef struct CompEmitter     CompEmitter;
typedef struct CompBody2d      CompBody2d;
typedef struct CompView2d      CompView2d;
typedef struct CompProjectile  CompProjectile;
typedef struct CompItem        CompItem;
typedef struct CompInventory   CompInventory;
typedef struct CompAbilitySlot CompAbilitySlot;
typedef struct CompTooltip     CompTooltip;

typedef struct ChainAttacks ChainAttacks;
typedef struct Inventory    Inventory;
typedef struct Dmgnumbers   Dmgnumbers;
typedef struct SolRibbon    SolRibbon;
typedef struct SolEvents    SolEvents;
typedef struct SolEmitters  SolEmitters;
typedef struct WorldPhysx   WorldPhysx;
typedef struct WorldLines   WorldLines;
typedef struct SolCamera    SolCamera;
typedef struct WorldNet     WorldNet;
typedef struct SolScore     SolScore;

typedef struct CompFlags
{
    EFlag flags;
} CompFlags;
typedef struct CompTracker
{
    World *world;
    u32    entId;
} CompTracker;

struct World
{
    SystemUpdate prestepSystems[MAX_SYSTEMS];
    SystemUpdate stepSystems[MAX_SYSTEMS];
    SystemUpdate poststepSystems[MAX_SYSTEMS];
    SystemUpdate tickSystems[MAX_SYSTEMS];
    SystemUpdate draw3dSystems[MAX_SYSTEMS];
    SystemUpdate draw2dSystems[MAX_SYSTEMS];
    SystemFunc   deinitSystems[MAX_SYSTEMS];

    int         activeEntities[MAX_ENTS];
    u32         gens[MAX_ENTS];
    Mask        masks[MAX_ENTS];
    EKind       ekinds[MAX_ENTS];
    CompFlags   flags[MAX_ENTS];
    CompTracker trackers[MAX_ENTS];

    // void *components[COMPONENT_COUNT];
    // CompXform *xforms;

    CompReplication *replications;
    CompParent      *parents;
    CompAudio       *audios;
    CompTimer       *timers;
    CompXform       *xforms;
    CompBody        *bodies;
    CompMovement    *movements;
    CompModel       *models;
    CompInteract    *interacts;
    CompShape       *spheres;
    CompVital       *vitals;
    CompController  *controllers;
    CompBuff        *buffs;
    CompAbility     *abilities;
    CompOwner       *owners;
    CompAi          *aicontrollers;
    CompCombat      *combats;
    CompEmitter     *compEmitters;
    CompBody2d      *body2d;
    CompView2d      *view2d;
    CompProjectile  *projectiles;
    CompTooltip     *tooltips;
    CompItem        *items;
    CompInventory   *inventories;
    CompAbilitySlot *abilitySlots;

    Dmgnumbers   *dmgNumbers;
    SolRibbon    *ribbon;
    ChainAttacks *chainhit;
    SolEvents    *events;
    WorldPhysx   *spatial;
    WorldLines   *lines;
    SolEmitters  *emitters;
    WorldNet     *worldNet;
    SolScore     *scores;

    bool doesSimulate;
    bool doesRender;

    int prestepCount;
    int stepCount;
    int poststepCount;
    int tickCount;
    int draw2dCount;
    int draw3dCount;
    int deinitCount;

    int activeCount;
    int playerID;
    int skyboxId;
    u32 systemBits;

    WorldKind kind;

    bool doesReplicate;
    u32  worldId;
    u32  currentTick;
};

World *World_Create(WorldKind kind);
World *World_Create_Default(WorldKind kind);
World *Sol_GetWorldById(u32 id);

void World_Destroy(World *world);
void World_System_Add(World *world, WorldSystem system);

int  Sol_World_GetEntCount(World *world);
void Sol_World_SetActive(World *world);
void Sol_World_SetReplicates(World *world, bool active);
void Sol_World_SetTracker(World *world, int id, World *otherWorld, int otherId);

void Worlds_Tick(World **worlds, int count, double dt, double time);
void Worlds_Step(World **worlds, int count, double dt, double time);
void Worlds_Draw3d(World **worlds, int count, double dt, double time);
void Worlds_Draw2d(World **worlds, int count, double dt, double time);

void Sol_Xform_Snapshot(World **worlds, int count);
void Sol_Xform_Interpolate(World **worlds, int count, float alpha);

int  Sol_Create_Ent(World *world, u32 id);
void Sol_Destroy_Ent(World *world, int id);

void Sol_Flags_Add(World *world, int id, EFlag flags);
void Sol_Flags_Remove(World *world, int id, EFlag flags);

#define ENTITY_INDEX_BITS 16
#define ENTITY_INDEX_MASK ((1U << ENTITY_INDEX_BITS) - 1)
#define EntIdx(id) (id & ENTITY_INDEX_MASK)
#define EntGen(id) (id >> ENTITY_INDEX_BITS)
#define EntIdxGen(id, gen) ((gen << ENTITY_INDEX_BITS) | (id & ENTITY_INDEX_MASK))
static inline u32 Sol_GetEntIndex(int id)
{
    return id & ENTITY_INDEX_MASK;
}

static inline u32 Sol_GetEntGen(int id)
{
    return id >> ENTITY_INDEX_BITS;
}

static inline u32 Sol_CreateEntGen(int id, int gen)
{
    return (gen << ENTITY_INDEX_BITS) | (id & ENTITY_INDEX_MASK);
}