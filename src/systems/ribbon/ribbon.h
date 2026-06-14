/*
 * File: ribbon.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-10
 */
#pragma once
#include "sol/types.h"

#define MAX_RIBBON_SEGS 32

typedef enum
{
    RIBBONKIND_BULLETTRAIL,
    RIBBONKIND_LIGHTNING,
    RIBBONKIND_LASER,
    RIBBONKIND_INFBEAM,
    RIBBONKIND_COUNT,
} RibbonKind;

typedef enum
{
    RIBBONATTACH_TRAIL,
    RIBBONATTACH_ENT_TO_ENT,
    RIBBONATTACH_ENT_TO_POS,
} RibbonAttachMode;

// Stable, data-oriented identifier given to gameplay systems
typedef struct RibbonHandle
{
    u32 index;      // Target slot inside the lookup table
    u32 generation; // Validation ticket to ensure slot wasn't recycled
} RibbonHandle;

typedef struct Ribbon
{
    RibbonHandle     handle; // Back-pointer to self-identity slot
    RibbonKind       kind;
    RibbonAttachMode attachMode;

    vec3s points[MAX_RIBBON_SEGS];
    float ages[MAX_RIBBON_SEGS];
    int   head, count, segments;

    float ttl, segLifetime, rate, accumulator, width, stretch;
    vec2s uv, uvv;
    vec4s color;

    u8    inf;
    u32   followId, targetId, textureId;
    vec3s targetPos;
} Ribbon;

// --- Lifecycle APIs ---
void         Sol_Ribbon_Init(World *world);
RibbonHandle Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s pos, vec3s posB, vec4s color);
Ribbon      *Sol_Ribbon_Get(World *world, RibbonHandle handle);
void         Sol_Ribbon_Kill(World *world, RibbonHandle handle);
bool         Sol_Ribbon_IsAlive(World *world, RibbonHandle handle);

// --- Gameplay Mutation APIs ---
void         Sol_Ribbon_UpdateTargetPos(World *world, RibbonHandle handle, vec3s newTargetPos);
RibbonHandle Sol_Ribbon_Add(World *world, int id, RibbonKind kind, float width, vec4s color);
RibbonHandle Sol_Ribbon_AddBetweenEntities(World *world, int entA, int entB, RibbonKind kind, float width, vec4s color);
RibbonHandle Sol_Ribbon_AddWithTarget(World *world, int id, RibbonKind kind, vec3s targetPos, float width, vec4s color);