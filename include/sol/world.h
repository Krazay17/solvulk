#pragma once
#include "sol/types.h"

#define MAX_ENTS 50000
#define MAX_SYSTEMS 64

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
    HAS_CAMERA = (1 << 10),
    HAS_CONTROLLER_AI = (1 << 11),
} CompBits;

typedef bool Active;
typedef uint32_t Mask;

typedef struct CompXform
{
    vec3s pos, lastPos, drawPos;
    vec4s rot, lastRot, drawRot;
    vec3s scale, lastScale, drawScale;
} CompXform;

typedef enum
{
    BODY_STATIC,
    BODY_DYNAMIC,
} BodyType;

typedef struct CompBody
{
    vec3s vel, impulse, force;
    BodyType type;
    float grounded, airtime;
    float radius, height;
    float mass, invMass, restitution;
} CompBody;

typedef enum
{
    SHAPE_RECTANGLE,
    SHAPE_TRIANGLE,
} ShapeType;

typedef struct CompShape
{
    ShapeType type;
    float width, height;
} CompShape;

typedef void (*InteractCallback)(void *data);
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
    uint32_t actionState;
    float yaw, pitch;
} CompController;

typedef struct AiController
{
    vec3s lookdir, wishdir;
    AiAction actionState;
} AiController;

SOLAPI World *World_Create(void);
SOLAPI World *World_Create_Default(void);
SOLAPI void World_Destroy(World *world);

SOLAPI int Entity_Create(World *world);
SOLAPI void Entity_Destroy(World *world, int id);

SOLAPI int Sol_Prefab_Button(World *world, vec3s pos, const char *text);
SOLAPI int Sol_Prefab_Wizard(World *world, vec3s pos);
SOLAPI int Sol_Prefab_Boxman(World *world, vec3s pos);

SOLAPI CompXform *Entity_Add_Xform(World *world, int id);
SOLAPI CompBody *Entity_Add_Body2(World *world, int id);
SOLAPI CompBody *Entity_Add_Body3(World *world, int id);
SOLAPI CompShape *Entity_Add_Shape(World *world, int id);
SOLAPI CompInteractable *Entity_Add_Interact(World *world, int id);
SOLAPI CompInfo *Entity_Add_Info(World *world, int id);
SOLAPI CompUiElement *Entity_Add_UiElement(World *world, int id);
SOLAPI CompMovement *Entity_Add_Movement(World *world, int id);
SOLAPI CompController *Entity_Add_Controller_Local(World *world, int id);
SOLAPI CompController *Entity_Add_Controller_Remote(World *world, int id);
SOLAPI CompController *Entity_Add_Controller_Ai(World *world, int id);
SOLAPI CompModel *Entity_Add_Model(World *world, int id);

int Sol_World_GetEntCount(World *world);