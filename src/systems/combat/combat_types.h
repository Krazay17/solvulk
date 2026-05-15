#pragma once
#include "sol/base.h"
#include "buff/buff_types.h"

typedef enum
{
    DAMAGEKIND_NORMAL,
    DAMAGEKIND_FIRE,
} DamageKind;
typedef struct SolHit
{
    DamageKind kind;
    BuffDesc  *buffs;
    u32        buffcount;
    u32        source, target;
    vec3s      pos, dir;
    float      power;
    u32        damage;
} SolHit;