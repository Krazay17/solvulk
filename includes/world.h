#pragma once
#include <stdbool.h>
#include "model.h"

#define MAX_ENTS 100

typedef void (*WorldInit)();
typedef void (*WorldTick)(double dt, double time);
typedef void (*WorldDraw)();

typedef struct
{
    WorldInit init;
    WorldTick tick;
    WorldDraw draw;
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

World *GetGame(void);