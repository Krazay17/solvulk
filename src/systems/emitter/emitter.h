#pragma once
#include "sol/types.h"

typedef enum
{
    PARTICLE_ORB,
    PARTICLE_FIRE,
    PARTICLE_SHOCK,
    PARTICLE_CLOUD,
    PARTICLE_BLOOD,
    PARTICLE_COUNT,
} ParticleKind;

typedef struct Particle
{
    ParticleKind kind;
    vec3s        pos, vel;
    vec4s        color;
    float        ttl, scale, span, speed;
    float        rot, rotspeed, offset, scalein, scaleout;
    u32          randScale, followId;
} Particle;

typedef struct Emitter
{
    EmitterKind emitterKind;
    vec3s       pos, vel;
    float       ttl, rate, accumulator;
    Particle    particle;
    u32         burst, inf, followId;
} Emitter;

void Sol_Emitter_Init(World *world);

void      Sol_Emitter_Add(World *world, Emitter e);
u32       Sol_Emitter_GetParticleCount(World *world);
Particle *Sol_Emitter_GetParticles(World *world);