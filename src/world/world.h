#pragma once
#include "solmath.h"

#define MAX_ENTS 2048
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
    HAS_BODY = (1 << 1),
    HAS_SHAPE = (1 << 2),
    HAS_INTERACT = (1 << 3),
    HAS_MODEL = (1 << 4),
    HAS_INFO = (1<<5),
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

typedef struct
{
    bool isHovered, isPressed, isClicked;
} CompInteractable;

typedef struct
{
    uint32_t gpuHandle;
} CompModel;

typedef struct
{
    char name[24];
    
} CompInfo;

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
void Entity_Add_Body(World *world, int id, CompBody body);
void Entity_Add_Shape(World *world, int id, CompShape shape);
void Entity_Add_Interact(World *world, int id, CompInteractable interact);
void Entity_Add_Info(World *world, int id, CompInfo info);