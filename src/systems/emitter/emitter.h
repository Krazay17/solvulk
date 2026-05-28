#pragma once
#include "sol/types.h"

typedef enum
{
    EMITTERKIND_FLASH_WHITEBALL,
    EMITTERKIND_FLASH_REDBALL,
    EMITTERKIND_FLASH_FIREBALL,
    EMITTERKIND_BURST_SPARKS,
    EMITTERKIND_BURST_CLOUDS,
} EmitterKind;

typedef enum
{
    PARTICLE_ORB,
    PARTICLE_FIRE,
    PARTICLE_SHOCK,
    PARTICLE_CLOUD,
    PARTICLE_BLOOD,
    PARTICLE_FIREBALL,
    PARTICLE_FIREBALL1,
    PARTICLE_COUNT,
} ParticleKind;

typedef struct Particle
{
    ParticleKind kind;
    vec3s        pos, vel;
    vec4s        color;
    float        ttl, scale, span, speed, delay;
    float        rot, rotspeed, offset, scalein, scaleout, fadein, fadeout;
    u32          randScale, followId, randLife, randScaleout;
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