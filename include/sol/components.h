/*
 * File: components.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Components!
 */
#pragma once
#include "sol/types.h"

#define MAX_VIEWS 8

// AUDIO----------------
typedef struct CompAudio CompAudio;
typedef struct
{
    bool doesLoop, is3D;
} AudioDesc;
void Sol_World_Audio_Init(World *world);
void Sol_World_Audio_Add(World *world, int id, AudioDesc desc);
void Sol_World_Audio_Step(World *world, double dt, double time);

// TIMER-----------------
typedef struct CompTimer CompTimer;
typedef struct
{
    float elapsed, duration;
} TimerDesc;
void Sol_Timer_Init(World *world);
void Sol_Timer_Add(World *world, int id, TimerDesc init);
void Sol_Timer_Step(World *world, double dt, double time);

// PICKUP---------------
void Sol_Pickup_Init(World *world);
void Sol_Pickup_Step(World *world, double dt, double time);

// SHAPE----------------
typedef struct CompShape CompShape;
typedef enum
{
    SHAPEKIND_SPHERE,
    SHAPEKIND_FIREBALL,
} ShapeKind;
typedef struct
{
    ShapeKind kind;
    float     radius;
    vec4s     color;
} ShapeDesc;
void Sol_Shape_Init(World *world);
void Sol_Shape_Add(World *world, int id, ShapeDesc desc);
void Sol_Shape_Draw(World *world, double dt, double time);
void Sol_Shape_ColorAll(World *world, vec4s color);

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
    u32        group, mask;
    u32        overlapGroup, overlapMask;
} CompBody2d;
void        Sol_Body2d_Init(World *world);
CompBody2d *Sol_Body2d_Add(World *world, int id, Body2dKind kind, float width, float height, u32 group, u32 mask);
vec2s       Sol_Body2d_GetDims(World *world, int id);
void        Sol_Body2d_SetOverlapMask(World *world, int id, u32 group, u32 mask);

typedef enum
{
    VIEW2DKIND_RECT,
    VIEW2DKIND_TEXT,
    VIEW2DKIND_CIRCLE,
    VIEW2DKIND_COUNT,
} View2dKind;
typedef struct
{
    View2dKind kind;
    vec4s      dims, offset;
    vec4s      color;
    vec4s      hoverColor, clickColor, toggleColor;
    float      fill, scale, textWidth, border;
    float      hoverAnim, clickAnim;
    float      targetFill;
    u32        zindex;
    u8         textureID, flags;
    vec2s      textureUV;
    char       text[64];
} SolView2d;
typedef struct CompView2d
{
    SolView2d views[MAX_VIEWS];
    u8        count;
    u8        zindex;
} CompView2d;

void        Sol_View2d_Init(World *world);
SolView2d *Sol_View2d_Add(World *world, int id, View2dKind kind, vec4s color, float width, float height);
CompView2d *Sol_View2d_Get(World *world, int id);
void        Sol_View2d_Set(World *world, int id, CompView2d view);
void        Sol_View2d_SetText(World *world, int id, SolView2d *view, const char *text);

typedef enum
{
    PROJECTILEKIND_BULLET,
    PROJECTILEKIND_FIREBALL,
    PROJECTILEKIND_COUNT,
} ProjectileKind;
typedef struct CompProjectile
{
    ProjectileKind kind;
    u32            bounces;
    float          power;
    float          explodeRadius;
    HitKind        directHitKind;
    HitKind        explosionHitKind;
} CompProjectile;
void            Sol_Projectile_Init(World *world);
CompProjectile *Sol_Projectile_Add(World *world, int id, ProjectileKind kind, float power);

typedef enum
{
    ITEMKIND_FIREBALL_CARD,
    ITEMKIND_BLASTER_CARD,
    ITEMKIND_SHIELD_CARD,
    ITEMKIND_ABILITY_CARD,
    ITEMKIND_ABILITY_SLOT,
    ITEMKIND_COUNT,
} ItemKind;
typedef struct CompItem
{
    ItemKind kind;
    u32      ability;
    u32      slot;
} CompItem;

void      Sol_Item_Init(World *world);
CompItem *Sol_Item_Add(World *world, int id, ItemKind kind);
void      Sol_Item_AddAbility(World *world, int id, u32 ability);
void      Sol_Item_AddAbilitySlot(World *world, int id, int slot);
