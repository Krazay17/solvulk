#pragma once
#include "base.h"

typedef struct World World;

typedef struct CompTimer
{
    float elapsed, duration;
} CompTimer;

void Sol_Timer_Init(World *world);
void Sol_Timer_Add(World *world, int id, CompTimer init);
void Sol_Timer_Step(World *world, double dt, double time);
