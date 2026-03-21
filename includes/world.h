#pragma once
#include <stdbool.h>
#include "systems.h"

#define MAX_ENTS 100

typedef void (*WorldInit)();
typedef void (*WorldStep)(double dt, double time);
typedef void (*WorldTick)(double dt, double time);
typedef void (*WorldDraw)();

typedef struct
{
    WorldInit init;
    WorldStep step;
    WorldTick tick;
    WorldDraw draw;
    bool active;
    void *state;
} World;

typedef struct
{
    int id;
    bool active;
} Entity;

typedef struct
{
    Entity entities[MAX_ENTS];
    int entCount;
} GameState;

World *Sol_GetGame(void);
World *Sol_GetMenu(void);