#pragma once
#include "emitter/emitter.h"
#include "sol/base.h"

#define MAX_IMPACTS 4
#define MAX_BUFFS_HIT 8

typedef struct SolHit
{
    HitKind kind;
    u32     buffKind[MAX_BUFFS_HIT];
    u32     buffcount;
    vec3s   pos, vel, normal;
    float   power;
    u32     damage;
    u32     entA, entB;

} SolHit;

typedef enum
{
    IMPACT_DIRECT,
    IMPACT_AOE,
} ImpactKind;

typedef struct
{
    ImpactKind kind;
    float      radius;  // only for AOE
    SolHit     hit;     // template — target/pos filled at impact time
    bool       falloff; // damage decreases with distance for AOE
    Emitter    emitter;
} Impact;

typedef struct
{
    Impact impacts[MAX_IMPACTS]; // e.g., 4
    int    impactCount;
} ImpactList;