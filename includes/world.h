#pragma once
#include "solmath.h"

#define MAX_ENTS 2048
#define MAX_SYSTEMS 64

typedef enum
{
    SYSTEM_STEP,
    SYSTEM_TICK,
    SYSTEM_DRAW,
} SystemType;

typedef enum
{
    HAS_NONE        = 0,
    HAS_XFORM       = (1 << 0),
    HAS_BODY        = (1 << 1),
    HAS_BUTTON      = (1 << 2),
} CompBits;

typedef bool Active;
typedef uint32_t Mask;

typedef struct
{
    vec3 pos;
} Xform;

typedef struct
{
    vec3 vel;
} Veloc;

typedef struct
{
    vec4 rect;
} CompRect;

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
    Xform xforms[MAX_ENTS];
    Veloc velocs[MAX_ENTS];
    CompRect rects[MAX_ENTS];

    bool worldActive;
};

World *World_Create(void);
void World_Destroy(World *world);

void World_Step(World *world, double dt, double time);
void World_Tick(World *world, double dt, double time);
void World_Draw(World *world, double dt, double time);

void World_System_Add(World *world, SystemFunc func, SystemType type);

int Entity_Create(World *world);
void Entity_Destroy(World *world, int id);

void Entity_Add_Xform(World *world, int id, Xform xform);
void Entity_Add_Veloc(World *world, int id, Veloc veloc);
void Entity_Add_Rect(World *world, int id, CompRect rect);