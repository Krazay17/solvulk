/*
 * File: components.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Components!
 */
#pragma once
#include "sol/types.h"

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
    vec2s      vel, dims, grav;
} CompBody2d;
void  Sol_Body2d_Init(World *world);
void  Sol_Body2d_Add(World *world, int id, CompBody2d desc);
vec2s Sol_Body2d_GetDims(World *world, int id);

typedef enum
{
    VIEW2DKIND_RECT,
    VIEW2DKIND_TEXT,
    VIEW2DKIND_CIRCLE,
    VIEW2DKIND_TEXTURE,
    VIEW2DKIND_COUNT,
} View2dKind;
typedef struct CompView2d
{
    View2dKind kind;
    vec2s      dims;
    vec4s      color;
    vec4s      hoverColor, clickColor, toggleColor;
    float      fill, scale, textWidth, border;
    float      hoverAnim, clickAnim;
    float      targetFill;
    u32        zindex;
    char       text[64];
} CompView2d;
void Sol_View2d_Init(World *world);
void Sol_View2d_Add(World *world, int id, CompView2d desc);

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
void Sol_Projectile_Init(World *world);
void Sol_Projectile_Add(World *world, int id, ProjectileKind kind, float power);