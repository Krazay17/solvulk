#pragma once
#include "emitter_types.h"
#include "sol/types.h"

#define MAX_PARTICLES 0xFFFFF
#define MAX_EMITTERS 0xFFFF
#define EMITTER_PARTICLES 64

typedef struct Particle
{
    ParticleKind kind;
    vec3s        pos, vel;
    vec4s        color;
    float        life, scale;
} Particle;

typedef struct Emitter
{
    EmitterKind  emitterKind;
    ParticleKind particleKind;
    vec3s        pos, vel;
    float        life, rate;

    u32 particleOffset;
    u32 particleCount;

    Particle *particles;
} Emitter;

typedef struct SolEmitters
{
    Emitter *emitter;
    u32      count;
    u32      capacity;

    Particle *particle_pool;
    u32       particle_cursor;
} SolEmitters;