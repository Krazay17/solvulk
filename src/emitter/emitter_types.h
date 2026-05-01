#pragma once
#include "sol/types.h"

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

typedef struct
{
    float rate, duration, spread;
} EmitterConfig;

typedef struct EmitterDesc
{
    EmitterKind   emitterKind;
    ParticleKind  particleKind;
    EmitterConfig overrides;
    vec3s         pos;
} EmitterDesc;