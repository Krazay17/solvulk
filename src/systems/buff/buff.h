#pragma once
#include "sol/types.h"

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
} BuffDesc;
void Sol_Buff_Init(World *world);
void Sol_Buff_Add(World *world, int id, BuffDesc desc);
void Sol_Buff_Remove(World *world, int id, BuffKind kind);
void Sol_Buff_Step(World *world, double dt, double time);
bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind);
