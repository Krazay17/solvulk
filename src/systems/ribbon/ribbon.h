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
#define MAX_RIBBON_SEGS 32
#define MAX_COMPRIBBONS 4

typedef enum
{
    RIBBONKIND_TRAIL,
    RIBBONKIND_LIGHTNING,
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
    vec3s points[MAX_RIBBON_SEGS]; // ring buffer of positions
    float ages[MAX_RIBBON_SEGS];   // age of each segment in seconds
    int   head;                    // newest point index
    int   count;                   // active segment count (capped at MAX_RIBBON_SEGS)

    float segLifetime; // how long each segment lives before fading out
    float rate;        // min distance between new points (0 = every frame)
    float accumulator; // distance accumulator since last point

    float width;
    float fadein;
    float fadeout;
    vec4s color;

    float ttl;      // ribbon lifetime; ignored if inf
    bool  inf;      // never expire the ribbon itself
    u32   followId; // if set, head point tracks this entity every tick
    bool  alive;

    u8    attachMode;
    u32   targetId;
    vec3s targetPos;
} Ribbon;

typedef struct CompRibbon
{
    Ribbon ribbons[MAX_COMPRIBBONS];
    u32    ribbonCount;
} CompRibbon;

void Sol_Ribbon_Init(World *world);

// World-spawned (fire-and-forget, like Sol_Emitter_Spawn)
void Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s pos, vec4s color);

// Entity-attached (like Sol_Emitter_Add — follows the entity each tick)
void Sol_Ribbon_Add(World *world, int id, RibbonKind kind, float width, vec4s color);
void Sol_Ribbon_AddBetweenEntities(World *world, int entA, int entB, RibbonKind kind, float width, vec4s color);
void Sol_Ribbon_AddToPosition(World *world, int entA, vec3s targetPos, RibbonKind kind, float width, vec4s color);