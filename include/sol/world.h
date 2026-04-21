#pragma once

#include "sol/types.h"

#define MAX_WORLDS 4
#define MAX_ENTS 50000
#define MAX_SYSTEMS 64

typedef struct World World;
typedef void (*SystemInit)(World *world);
typedef void (*SystemFunc)(World *world, double dt, double time);
typedef void (*InteractCallback)(void *data);
typedef bool Active;
typedef uint32_t Mask;

// fwds
typedef struct WorldSpatial WorldSpatial;
typedef struct WorldLines WorldLines;

typedef enum
{
    WORLD_SYS_PHYSX,
    WORLD_SYS_CAM,
    WORLD_SYS_CONTROLLER_LOCAL,
    WORLD_SYS_CONTROLLER_AI,
    WORLD_SYS_INTERACT,
    WORLD_SYS_MODEL,
    WORLD_SYS_UI,
    WORLD_SYS_MOVEMENT,
    WORLD_SYS_LINE,
    WORLD_SYS_COMBAT,
    WORLD_SYS_COUNT,
} WorldSystems;

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

typedef struct CompBody
{
    vec3s vel, impulse, force;
    Shape3 shape;
    float grounded, airtime;
    float radius, height, length;
    float mass, invMass, restitution;
} CompBody;

typedef struct
{
    SystemInit init;
    SystemFunc step;
    SystemFunc tick;
    SystemFunc draw;
} SystemFuncs;

typedef struct World
{
    bool worldActive;
    SystemFunc stepSystems[MAX_SYSTEMS];
    int stepCount;
    SystemFunc tickSystems[MAX_SYSTEMS];
    int tickCount;
    SystemFunc drawSystems[MAX_SYSTEMS];
    int drawCount;

    WorldSpatial *spatial;
    WorldLines *lines;

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
void World_System_Add(World *world, WorldSystems system);
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
SOLAPI CompShape *Entity_Add_Shape(World *world, int id);
SOLAPI CompBody *Entity_Add_Body2(World *world, int id);
SOLAPI CompBody *Entity_Add_Body3(World *world, int id, CompBody body);
SOLAPI CompInteractable *Entity_Add_Interact(World *world, int id);
SOLAPI CompInfo *Entity_Add_Info(World *world, int id);
SOLAPI CompUiElement *Entity_Add_UiElement(World *world, int id);
SOLAPI CompMovement *Entity_Add_Movement(World *world, int id);
SOLAPI CompController *Entity_Add_Controller_Local(World *world, int id);
SOLAPI CompController *Entity_Add_Controller_Remote(World *world, int id);
SOLAPI CompController *Entity_Add_Controller_Ai(World *world, int id);
SOLAPI CompModel *Entity_Add_Model(World *world, int id, SolModelId model);
SOLAPI CompCombat *Sol_Add_Combat(World *world, int id);


// Xform systems
SOLAPI void Sol_System_Xform_Snapshot(World *world);
SOLAPI void Sol_System_Xform_Interpolate(World *world, float alpha);
// Init Systems
SOLAPI void Physx_Init(World *world);
SOLAPI void Lines_Init(World *world);
// Step Systems
SOLAPI void Sol_System_Movement_2d_Step(World *world, double dt, double time);
SOLAPI void Sol_System_Movement_3d_Step(World *world, double dt, double time);
SOLAPI void Sol_System_Step_Physx_2d(World *world, double dt, double time);
SOLAPI void Sol_System_Step_Physx_3d(World *world, double dt, double time);
// Tick Systems
SOLAPI void Sol_System_Info_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Interact_Ui(World *world, double dt, double time);
SOLAPI void Sol_System_Controller_Local_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Controller_Ai_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Camera_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Line_Tick(World *world, double dt, double time);
SOLAPI void System_Combat_Tick(World *world, double dt, double time);
// Draw Systems
SOLAPI void Sol_System_Model_Draw(World *world, double dt, double time);
SOLAPI void Sol_System_Line_Draw(World *world, double dt, double time);
SOLAPI void Sol_System_UI_Draw(World *world, double dt, double time);
// Draw Calls
SOLAPI void Sol_Draw_Model_Instanced(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
SOLAPI void Sol_Draw_Rectangle(SolRect rect, SolColor color, float thickness);
SOLAPI void Sol_Draw_Line(SolLine *lines, int count);
SOLAPI void Sol_Draw_Text(const char *str, float x, float y, float size, SolColor color);

static SystemFuncs world_systems[WORLD_SYS_COUNT] = {
    [WORLD_SYS_PHYSX] = {.init = Physx_Init, .step = Sol_System_Step_Physx_3d},
    [WORLD_SYS_CAM] = {.tick = Sol_System_Camera_Tick},
    [WORLD_SYS_CONTROLLER_LOCAL] = {.tick = Sol_System_Controller_Local_Tick},
    [WORLD_SYS_CONTROLLER_AI] = {.tick = Sol_System_Controller_Ai_Tick},
    [WORLD_SYS_MODEL] = {.draw = Sol_System_Model_Draw},
    [WORLD_SYS_UI] = {.draw = Sol_System_UI_Draw},
    [WORLD_SYS_INTERACT] = {.tick = Sol_System_Interact_Ui},
    [WORLD_SYS_MOVEMENT] = {.step = Sol_System_Movement_3d_Step},
    [WORLD_SYS_LINE] = {.init=Lines_Init, .tick = Sol_System_Line_Tick, .draw = Sol_System_Line_Draw},
    [WORLD_SYS_COMBAT] = {.tick = System_Combat_Tick},
};