#pragma once
#include "base.h"

typedef struct World World;

typedef enum
{
    BODY2DKIND_RECT,
    BODY2DKIND_COUNT,
} Body2dKind;
typedef struct CompBody2d
{
    Body2dKind kind;
    vec2s      vel, dims, grav, grabPos;
    u32        overlapping[4];
    u32        overlapCount;
    u32        group, mask, zindex;
    u32        overlapGroup, overlapMask;
} CompBody2d;

void        Sol_Body2d_Init(World *world);
CompBody2d *Sol_Body2d_Add(World *world, int id, Body2dKind kind, float width, float height, u32 group, u32 mask);
vec2s       Sol_Body2d_GetDims(World *world, int id);
void        Sol_Body2d_SetOverlapMask(World *world, int id, u32 group, u32 mask);
