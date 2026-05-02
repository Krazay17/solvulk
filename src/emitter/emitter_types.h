#pragma once
#include "sol/base_types.h"

typedef enum
{
    PARTICLE_ORB,
    PARTICLE_COUNT,
} ParticleKind;

typedef enum
{
    EMITTER_FOUNTAIN,
    EMITTER_COUNT,
} EmitterKind;

typedef struct EmitterDesc
{
    // EmitterKind  emitterKind;
    // ParticleKind particleKind;
    vec3s pos, rot, vel;
    vec4s color;
    float ttl, pttl, rate;
    u32   burst;
} EmitterDesc;

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
    u32         burst;
} Emitter;