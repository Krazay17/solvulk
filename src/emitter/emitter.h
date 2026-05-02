#pragma once
#include "sol/emitter_types.h"
#include "sol/types.h"

#define MAX_EMITTERS 0xFFF
#define MAX_PARTICLES 0xFFFF

typedef struct SolEmitters
{
    Emitter *emitter;
    u32      emitter_count;
    u32      emitter_capacity;

    Particle *particle_pool;
    u32       particle_count;
    u32       particle_capacity;
} SolEmitters;

Particle *Particle_Activate(SolEmitters *sys, Emitter *init);
void Particle_Tick(World *world, double dt, double time);