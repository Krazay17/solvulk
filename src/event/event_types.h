#pragma once
#include "sol/base_types.h"

typedef enum
{
    EVENT_NONE,
    EVENT_PARTICLE,
    EVENT_COLLISION,
    EVENT_COUNT,
} EventKind;

typedef struct EventDesc
{
    EventKind kind;
    u32 entA;
    vec3s pos, normal;
} EventDesc;