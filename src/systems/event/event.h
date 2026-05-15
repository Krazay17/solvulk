#pragma once
#include "sol/types.h"

typedef enum
{
    EVENT_COLLISION,
    EVENT_DEATH,
} EventKind;
typedef struct
{
    EventKind kind;
    union {
        struct
        {
            u32   entA, entB;
            vec3s pos, normal, vel;
        } collision;
        struct
        {
            int   attacker;
            float damage;
        } Death;
        SolHit hit;
    } as;
} SolEvent;

typedef struct SolEvents
{
    SolEvent *event;

    u32 count;
    u32 capacity;
} SolEvents;

void Sol_Event_Add(World *world, SolEvent event);
void Sol_Event_Init(World *world);
void Sol_Event_Clear(World *world);