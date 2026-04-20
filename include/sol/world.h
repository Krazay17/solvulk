#pragma once

#include "sol/types.h"

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
    uint32_t actionState;
    float yaw, pitch;
} CompController;


SOLAPI World *World_Create(void);
SOLAPI World *World_Create_Default(void);
SOLAPI void World_Destroy(World *world);
SOLAPI void World_System_Add(World *world, SystemFunc func, SystemKind kind);
SOLAPI int Sol_World_GetEntCount(World *world);

// Add thing to world
SOLAPI void Sol_World_Line_Add(World *world, vec3s a, vec3s b, vec3s color, float ttl);

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

