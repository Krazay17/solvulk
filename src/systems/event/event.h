#pragma once
#include "combat/combat_types.h"
#include "sol/types.h"

typedef enum
{
    EVENTKIND_COLLISION,
    EVENTKIND_HIT,
    EVENTKIND_FX,
    EVENTKIND_DEATH,
    EVENTKIND_COUNT,
} EventKind;
typedef struct SolEvent
{
    EventKind kind;
    union {
        SolHit hit;
        struct
        {
            vec3s pos, normal, vel;
            u32   entA, entB;
        } collision;
        struct
        {
            float damage;
            u32   entA, entB;
        } death;
        struct
        {
            u32   entA, entB;
            vec3s pos, rot;
            u32   kind;
            float scale;
        } fx;
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