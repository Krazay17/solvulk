#pragma once
#include "combat/combat_types.h"
#include "sol/types.h"

typedef enum
{
    EVENTKIND_COLLISION,
    EVENTKIND_HIT,
    EVENTKIND_FX,
    EVENTKIND_DEATH,
} EventKind;
typedef struct SolEvent
{
    EventKind kind;
    u32       sourceId, targetId;
    union {
        SolHit hit;
        struct
        {
            vec3s pos, normal, vel;
            u32   entA, entB;
        } collision;
        struct
        {
            int   attacker;
            float damage;
        } death;
        struct
        {
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