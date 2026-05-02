#pragma once
#include "event_types.h"
#include "sol/types.h"

#define EVENT_INIT_SIZE 12


typedef void (*EventFunc)(void *data);
extern EventFunc event_funcs[EVENT_COUNT];

typedef struct SolEvent
{
    u32   kind;
    u32   entA, entB;
    vec3s pos, normal;
} SolEvent;

typedef struct SolEvents
{
    SolEvent *event;

    u32 count;
    u32 capacity;
} SolEvents;

void Sol_Event_Add(World *world, EventDesc desc);
void Event_Init(World *world);
void Event_Clear(World *world);