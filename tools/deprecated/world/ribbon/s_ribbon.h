/*
 * File: ribbon.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-10
 */
#pragma once
#include "sol/types.h"

typedef struct World World;

#define MAX_RIBBON_SEGS 32

typedef enum
{
    RIBBONKIND_BULLETTRAIL,
    RIBBONKIND_LIGHTNING,
    RIBBONKIND_LASER,
    RIBBONKIND_INFBEAM,
    RIBBONKIND_COUNT,
} RibbonKind;

typedef struct RibbonHandle
{
    u32 index;      // Target slot inside the lookup table
    u32 generation; // Validation ticket to ensure slot wasn't recycled
} RibbonHandle;

// --- Lifecycle APIs ---
void         Sol_Ribbon_Init(World *world);
RibbonHandle Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s pos, vec3s posB, vec4s color, float width);
void         Sol_Ribbon_Kill(World *world, RibbonHandle handle);
bool         Sol_Ribbon_IsAlive(World *world, RibbonHandle handle);

// --- Gameplay Mutation APIs ---
void         Sol_Ribbon_UpdateTargetPos(World *world, RibbonHandle handle, vec3s newTargetPos);
void         Sol_Ribbon_UpdatePositions(World *world, RibbonHandle handle, vec3s startPos, vec3s endPos);
RibbonHandle Sol_Ribbon_Add(World *world, int id, RibbonKind kind, float width, vec4s color);
RibbonHandle Sol_Ribbon_AddBetweenEntities(World *world, int entA, int entB, RibbonKind kind, float width, vec4s color);
RibbonHandle Sol_Ribbon_AddWithTarget(World *world, int id, RibbonKind kind, vec3s targetPos, float width, vec4s color);