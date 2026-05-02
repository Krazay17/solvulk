#pragma once
#include "emitter_types.h"
#include "sol/types.h"

#define MAX_PARTICLES 0xFFFF
#define MAX_EMITTERS 0xFFFF
#define EMITTER_PARTICLES 64

typedef struct Particle
{
    ParticleKind kind;
    vec3s        pos, vel;
    vec4s        color;
    float        ttl, scale, startTtl;
} Particle;

typedef struct Emitter
{
    EmitterKind emitterKind;
    vec3s       pos, vel;
    float       ttl, rate, accumulator;
    Particle    particle;

} Emitter;

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