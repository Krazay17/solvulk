#pragma once
#include "sol/base.h"

typedef struct World World;

typedef void (*StateUpdate)(World *world, int id, float dt);
typedef void (*StateEnter)(World *world, int id);
typedef void (*StateExit)(World *world, int id);
typedef bool (*StateCanExit)(World *world, int id);
typedef bool (*StateCanEnter)(World *world, int id);

typedef struct
{
    StateUpdate   update;
    StateEnter    enter;
    StateExit     exit;
    StateCanExit  canExit;
    StateCanEnter canEnter;
} StateFunc;

// void State_Update(World *world, int id, float dt);
// void State_Enter(World *world, int id);
// void State_Exit(World *world, int id);
// bool State_CanEnter(World *world, int id);
// bool State_CanExit(World *world, int id);
