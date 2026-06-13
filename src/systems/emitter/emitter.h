#pragma once
#include "sol/types.h"

#define MAX_COMPEMITTERS 8

typedef enum
{
    EMITTERKIND_FLASH_BALL,
    EMITTERKIND_FLASH_FIREBALL,
    EMITTERKIND_SINGLE_SPARK,
    EMITTERKIND_BURST_SPARKS,
    EMITTERKIND_BURST_CLOUDS,
    EMITTERKIND_BURST_FIRE,
    EMITTERKIND_FOUNTAIN_FIRE,
    EMITTERKIND_FOUNTAIN_FOG,
    EMITTERKIND_FOUNTAIN_SPARKS,
    EMITTERKIND_COUNT,
} EmitterKind;

typedef enum
{
    PARTICLE_ORB,
    PARTICLE_FIRE,
    PARTICLE_SHOCK,
    PARTICLE_CLOUD,
    PARTICLE_BLOOD,
    PARTICLE_FIREBALL,
    PARTICLE_SPARKFRONT,
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
    u32         burst, inf, followId, rateBurst;
} Emitter;

typedef struct CompEmitter
{
    Emitter emitters[MAX_COMPEMITTERS];
    u32     emitterCount;
} CompEmitter;

typedef void (*ParticleFunc)(Particle, double, double);

const extern Emitter emitter_kinds[EMITTERKIND_COUNT];

void Sol_Emitter_Init(World *world);

Emitter *Sol_Emitter_Spawn(World *world, EmitterKind kind, vec3s pos, vec4s color, float scale);
Emitter *Sol_Emitter_Add(World *world, int id, EmitterKind kind, vec4s color, float scale);
Emitter *Sol_Emitter_SpawnEx(World *world, Emitter src);

u32       Sol_Emitter_GetParticleCount(World *world);
Particle *Sol_Emitter_GetParticles(World *world);