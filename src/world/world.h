#pragma once
#include "solmath.h"

#define MAX_ENTS 50000
#define MAX_SYSTEMS 64

typedef enum
{
    SYSTEM_STEP,
    SYSTEM_TICK,
    SYSTEM_DRAW,
} SystemKind;

typedef enum
{
    HAS_NONE = 0,
    HAS_XFORM = (1 << 0),
    HAS_BODY2 = (1 << 1),
    HAS_BODY3 = (1 << 2),
    HAS_SHAPE = (1 << 3),
    HAS_INTERACT = (1 << 4),
    HAS_MODEL = (1 << 5),
    HAS_INFO = (1 << 6),
    HAS_UI_ELEMENT = (1 << 7),
    HAS_MOVEMENT = (1 << 8),
    HAS_CONTROLLER = (1 << 9),
} CompBits;

typedef bool Active;
typedef uint32_t Mask;

typedef struct
{
    vec3s pos;
    vec3s rot;
    vec3s scale;
} CompXform;

typedef struct
{
    vec3s vel;
    float width, height, mass;
} CompBody;

typedef enum
{
    SHAPE_RECTANGLE,
    SHAPE_TRIANGLE,
} ShapeType;
typedef struct
{
    ShapeType type;
    float width, height;
} CompShape;

typedef void (*InteractCallback)();
typedef struct
{
    bool isHovered, isPressed, isClicked;
    InteractCallback callback;
} CompInteractable;

typedef struct
{
    uint32_t gpuHandle;
} CompModel;

typedef struct
{
    char name[24];

} CompInfo;

typedef struct
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

typedef struct
{
    vec3s wishdir, updir;
    float gSpeed, gAccell, gFriction;
    float aSpeed, aAccell, aFriction;
} CompMovement;

typedef struct
{
    uint32_t actionState;
    float yaw, pitch;
} CompController;

typedef struct World World;
typedef void (*SystemFunc)(World *world, double dt, double time);

struct World
{
    SystemFunc stepSystems[MAX_SYSTEMS];
    int stepCount;
    SystemFunc tickSystems[MAX_SYSTEMS];
    int tickCount;
    SystemFunc drawSystems[MAX_SYSTEMS];
    int drawCount;

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

    bool worldActive;
};

World *World_Create(void);
void World_Destroy(World *world);

void World_Step(World *world, double dt, double time);
void World_Tick(World *world, double dt, double time);
void World_Draw(World *world, double dt, double time);

void World_System_Add(World *world, SystemFunc func, SystemKind kind);

int Entity_Create(World *world);
void Entity_Destroy(World *world, int id);

void Entity_Add_Xform(World *world, int id, CompXform xform);
void Entity_Add_Body2(World *world, int id, CompBody body);
void Entity_Add_Body3(World *world, int id, CompBody body);
void Entity_Add_Shape(World *world, int id, CompShape shape);
void Entity_Add_Interact(World *world, int id, CompInteractable interact);
void Entity_Add_Info(World *world, int id, CompInfo info);
void Entity_Add_UiElement(World *world, int id, CompUiElement uiElement);
void Entity_Add_Movement(World *world, int id, CompMovement movement);
void Entity_Add_Controller(World *world, int id, CompController controller);

CompXform Entity_Get_Xform(World *world, int id);
CompBody Entity_Get_Body2(World *world, int id);
CompBody Entity_Get_Body3(World *world, int id);
CompShape Entity_Get_Shape(World *world, int id);
CompInteractable Entity_Get_Interact(World *world, int id);
CompInfo Entity_Get_Info(World *world, int id);
CompUiElement Entity_Get_UiElement(World *world, int id);
CompMovement Entity_Get_Movement(World *world, int id);
CompController Entity_Get_Controller(World *world, int id);

void Sol_System_Button_Update(World *world, double dt, double time);
void Sol_System_Interact_Ui(World *world, double dt, double time);
void Sol_System_Update_View(World *world, double dt, double time);
void Sol_System_Step_Physx_2d(World *world, double dt, double time);
void Sol_System_Step_Physx_3d(World *world, double dt, double time);
void Sol_System_Info_Tick(World *world, double dt, double time);
void Sol_System_UI_Draw(World *world, double dt, double time);
void Sol_System_Movement_2d_Step(World *world, double dt, double time);
void Sol_System_Controller_Tick(World *world, double dt, double time);

int Sol_Prefab_Button(World *world, vec3s pos);
int Sol_Prefab_Wizard(World *world, vec3s pos);
int Sol_Prefab_Boxman(World *world, vec3s pos);