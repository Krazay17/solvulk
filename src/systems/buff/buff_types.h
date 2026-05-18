#pragma once
#include "sol/base.h"

typedef enum
{
    BUFFADD_ADD_DURATION,
    BUFFADD_SET_DURATION,
    BUFFADD_MULTIPLY,
    BUFFADD_INF,
} BuffAdd;

typedef enum
{
    BUFFKIND_KNOCKBACK,
    BUFFKIND_FIRE,
    BUFFKIND_SPEED,
    BUFFKIND_COUNT,
} BuffKind;

typedef struct
{
    BuffKind kind;
    BuffAdd  addKind;
    float    duration, freq;
    u32      initialDamage;
} BuffDesc;
