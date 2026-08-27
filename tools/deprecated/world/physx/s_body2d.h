#pragma once
#include "sol/base.h"

typedef struct World World;

typedef struct CompBody2d
{
    Shape2 kind;
    vec2s      vel, dims, grav, grabPos;
    u32        group, zindex;
    u32        overlap_group;
} CompBody2d;

void        Sol_Body2d_Init(World *world);
CompBody2d *Sol_Body2d_Add(World *world, int id, Shape2 kind, float width, float height, u32 group);
CompBody2d *Sol_Body2d_Get(World *world, int id);
bool        Sol_Body2d_DoesCollide(World *world, int id, int idB);
vec2s       Sol_Body2d_GetDims(World *world, int id);
void        Sol_Body2d_SetOverlapMask(World *world, int id, u32 group);
void        Sol_Body2d_SetVel(World *world, int id, vec2s vel);
int         Sol_Body2d_GetOverlapping(World *world, int id, int *overlapping_ents, int max);
