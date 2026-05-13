#pragma once
#include "sol/types.h"

typedef struct Particle
{
    ParticleKind kind;
    vec3s        pos, vel;
    vec4s        color;
    float        ttl, scale, span;
} Particle;

typedef struct Emitter
{
    EmitterKind emitterKind;
    vec3s       pos, vel;
    float       ttl, rate, accumulator;
    Particle    particle;
    u32         burst;
} Emitter;

void Sol_Emitter_Init(World *world);
void Sol_Emitter_Add(World *world, Emitter e);
void Emitter_Step(World *world, double dt, double time);
void Emitter_Tick(World *world, double dt, double time);
void Emitter_Draw(World *world, double dt, double time);