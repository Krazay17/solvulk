#pragma once
#include "sol/types.h"

#include "emitter.h"

void Emitter_Step(World *world, double dt, double time);
void Emitter_Tick(World *world, double dt, double time);
void Emitter_Draw(World *world, double dt, double time);