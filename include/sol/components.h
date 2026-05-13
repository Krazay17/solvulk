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

// SPHERE----------------
typedef struct CompShape CompShape;

void Sol_Shape_Init(World *world);
void Sol_Shape_Add(World *world, int id, ShapeDesc desc);
void Sol_Shape_Draw(World *world, double dt, double time);
void Sol_Shape_ColorAll(World *world, vec4s color);

// TIMER-----------------
typedef struct CompTimer CompTimer;
typedef struct
{
    float elapsed, duration;
} TimerDesc;
void Sol_Timer_Init(World *world);
void Sol_Timer_Add(World *world, int id, TimerDesc init);
void Sol_Timer_Tick(World *world, double dt, double time);

// BUFF-----------------
typedef struct CompBuff CompBuff;
typedef enum
{
    BUFF_KNOCKBACK = (1 << 0),
    BUFF_FIRE      = (1 << 1),
    BUFF_COUNT,
} BuffKind;
typedef struct
{
    BuffKind kind;
    float    duration;
} BuffDesc;
void Sol_Buff_Init(World *world);
void Sol_Buff_Add(World *world, int id, BuffDesc desc);
void Sol_Buff_Remove(World *world, int id, BuffKind kind);
void Sol_Buff_Step(World *world, double dt, double time);

// PICKUP---------------
void Sol_Pickup_Init(World *world);
void Sol_Pickup_Step(World *world, double dt, double time);

// COMBAT----------------
void Sol_Combat_Init(World *world);
void Sol_Combat_Step(World *world, double dt, double time);
