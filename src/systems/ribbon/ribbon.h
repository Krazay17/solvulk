/*
 * File: ribbon.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-10
 *
 */
#pragma once
#include "sol/types.h"

#define MAX_RIBBONS 512
#define MAX_RIBBON_SEGS 24
#define MAX_COMPRIBBONS 4

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
    RIBBONATTACH_TRAIL,      // Standard mode: record points over time
    RIBBONATTACH_ENT_TO_ENT, // Beam mode: from followId to targetId
    RIBBONATTACH_ENT_TO_POS, // Beam mode: from followId to targetPos
} RibbonAttachMode;

typedef struct Ribbon
{
    RibbonKind       kind;
    RibbonAttachMode attachMode;

    vec3s points[MAX_RIBBON_SEGS];
    float ages[MAX_RIBBON_SEGS];
    int   head, count, segments;

    float ttl, segLifetime, rate, accumulator, width, stretch;
    vec2s uv, uvv;
    vec4s color;

    u8  inf;
    u32 followId, targetId;
    vec3s targetPos;
} Ribbon;

typedef struct CompRibbon
{
    Ribbon ribbons[MAX_COMPRIBBONS];
    u32    ribbonCount;
} CompRibbon;

void Sol_Ribbon_Init(World *world);

Ribbon *Sol_Ribbon_Add(World *world, int id, RibbonKind kind, float width, vec4s color);
Ribbon *Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s pos, vec3s posB, vec4s color);
Ribbon *Sol_Ribbon_AddBetweenEntities(World *world, int entA, int entB, RibbonKind kind, float width, vec4s color);