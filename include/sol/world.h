#pragma once

#include "sol/types.h"
#include "internal_types.h"

#define MAX_WORLDS 4
#define MAX_ENTS 50000
#define MAX_SYSTEMS 64

typedef struct World World;
typedef void (*SystemFunc)(World *world, double dt, double time);
typedef void (*InteractCallback)(void *data);
typedef bool Active;
typedef uint32_t Mask;

// clang-format off
typedef enum
{
    HAS_NONE                =   0,
    HAS_XFORM               =   (1 << 0),
    HAS_BODY2               =   (1 << 1),
    HAS_BODY3               =   (1 << 2),
    HAS_SHAPE               =   (1 << 4),
    HAS_INTERACT            =   (1 << 5),
    HAS_MODEL               =   (1 << 6),
    HAS_INFO                =   (1 << 7),
    HAS_UI_ELEMENT          =   (1 << 8),
    HAS_MOVEMENT            =   (1 << 9),
    HAS_CONTROLLER          =   (1 << 10),
    HAS_CAMERA              =   (1 << 11),
    HAS_CONTROLLER_AI       =   (1 << 12),
    HAS_COMBAT              =   (1 << 13),
} CompBits;
// clang-format on

typedef enum
{
    SYSTEM_STEP,
    SYSTEM_TICK,
    SYSTEM_DRAW,
} SystemKind;

typedef enum CombatState
{
    COMBAT_IDLE,
    COMBAT_CHARGING,
    COMBAT_ATTACKING,
} CombatState;

typedef struct CompCombat
{
    CombatState state;
} CompCombat;

typedef struct CompXform
{
    vec3s pos, lastPos, drawPos;
    versors quat, lastQuat, drawQuat;
    vec3s scale, lastScale, drawScale;
} CompXform;

typedef struct CompBody
{
    vec3s vel, impulse, force;
    Shape3 shape;
    float grounded, airtime;
    float radius, height, length;
    float mass, invMass, restitution;
} CompBody;

typedef struct CompShape
{
    Shape2 type;
    float width, height;
} CompShape;

typedef struct CompInteractable
{
    bool isHovered, isPressed, isClicked, onHold;
    InteractCallback callback;
    void *callbackData;
} CompInteractable;

typedef struct CompModel
{
    uint32_t gpuHandle;
    SolModel *model;
    float yOffset;
} CompModel;

typedef struct CompInfo
{
    char name[24];

} CompInfo;

typedef struct CompUiElement
{
    SolColor baseColor;
    SolColor textColor;
    float fontSize;
    char text[64];
    float textWidth;

    float hoverAnim;
    float clickAnim;

    float borderThickness;
    SolColor borderColor;
} CompUiElement;

typedef struct CompMovement
{
    vec3s wishdir, updir, lockdir;
    float stateTimer;
    MoveState moveState;
    MoveConfigId configId;
} CompMovement;

typedef struct CompController
{
    vec3s lookdir, wishdir;
    vec2s wishdir2;
    PlayerActionStates actionState;
    float yaw, pitch;
} CompController;

typedef struct World
{
    bool worldActive;
    SystemFunc stepSystems[MAX_SYSTEMS];
    int stepCount;
    SystemFunc tickSystems[MAX_SYSTEMS];
    int tickCount;
    SystemFunc drawSystems[MAX_SYSTEMS];
    int drawCount;

    WorldSpatial spatial;
    WorldLines lines;

    int playerID;
    int activeEntities[MAX_ENTS];
    int activeCount;

    Active actives[MAX_ENTS];
    Mask masks[MAX_ENTS];

    CompXform xforms[MAX_ENTS];
    CompBody bodies[MAX_ENTS];
    CompShape shapes[MAX_ENTS];
    CompModel models[MAX_ENTS];
    CompInteractable interactables[MAX_ENTS];
    CompInfo infos[MAX_ENTS];
    CompUiElement uiElements[MAX_ENTS];
    CompMovement movements[MAX_ENTS];
    CompController controllers[MAX_ENTS];
    CompCombat combats[MAX_ENTS];
} World;

SOLAPI World *World_Create(void);
SOLAPI World *World_Create_Default(void);
SOLAPI void World_Destroy(World *world);
SOLAPI void World_System_Add(World *world, SystemFunc func, SystemKind kind);
SOLAPI int Sol_World_GetEntCount(World *world);

// Add thing to world
SOLAPI void Sol_World_Line_Add(World *world, vec3s a, vec3s b, vec3s color, vec3s bColor, float ttl);

// Add Prefab entity
SOLAPI int Sol_Prefab_Button(World *world, vec3s pos, const char *text);
SOLAPI int Sol_Prefab_Wizard(World *world, vec3s pos);
SOLAPI int Sol_Prefab_Boxman(World *world, vec3s pos);

// Add Entity
SOLAPI int Entity_Create(World *world);
SOLAPI void Entity_Destroy(World *world, int id);

// Add Component to entity
SOLAPI CompXform *Entity_Add_Xform(World *world, int id, vec3s pos);
SOLAPI CompBody *Entity_Add_Body2(World *world, int id);
SOLAPI CompBody *Entity_Add_Body3(World *world, int id, CompBody body);
SOLAPI CompShape *Entity_Add_Shape(World *world, int id);
SOLAPI CompInteractable *Entity_Add_Interact(World *world, int id);
SOLAPI CompInfo *Entity_Add_Info(World *world, int id);
SOLAPI CompUiElement *Entity_Add_UiElement(World *world, int id);
SOLAPI CompMovement *Entity_Add_Movement(World *world, int id);
SOLAPI CompController *Entity_Add_Controller_Local(World *world, int id);
SOLAPI CompController *Entity_Add_Controller_Remote(World *world, int id);
SOLAPI CompController *Entity_Add_Controller_Ai(World *world, int id);
SOLAPI CompModel *Entity_Add_Model(World *world, int id, SolModelId model);
SOLAPI CompCombat *Sol_Add_Combat(World *world, int id);