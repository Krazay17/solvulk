#pragma once
#include "buff/buff_types.h"
#include "emitter/emitter.h"
#include "sol/base.h"

#define MAX_IMPACTS 4
#define MAX_BUFFS_HIT 8

typedef enum
{
    DAMAGEKIND_NORMAL,
    DAMAGEKIND_FIRE,
    DAMAGEKIND_ICE,
} DamageKind;
typedef struct SolHit
{
    DamageKind kind;
    BuffDesc   buffs[MAX_BUFFS_HIT];
    u32        buffcount;
    u32        source, target;
    vec3s      pos, dir, vel;
    float      power;
    u32        damage;
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