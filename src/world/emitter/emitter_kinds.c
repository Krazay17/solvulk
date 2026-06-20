#include "s_emitter.h"

const Emitter emitter_kinds[EMITTERKIND_COUNT] = {
    [EMITTERKIND_POP_FIRE] =
        {
            .burst = 10,
            .particle =
                {
                    .kind     = PARTICLE_FIRE,
                    .ttl      = 1.0f,
                    .color    = {.85f, 0.07f, 0.05f, 0.7f},
                    .scale    = 1.0f,
                    .speed    = 1.0f,
                    .scalein  = .2f,
                    .scaleout = .2f,
                },
        },
    [EMITTERKIND_SINGLE_SPARK] =
        {
            .burst = 1,
            .particle =
                {
                    .kind     = PARTICLE_SHOCK_ADD,
                    .ttl      = 0.2f,
                    .scale    = 1.0f,
                    .scaleout = 0.9f,
                    .color    = {1.0f, 1.0f, 1.0f, .88f},
                },
        },
    [EMITTERKIND_FLASH_BALL] =
        {
            .burst = 1,
            .particle =
                {
                    .scale    = 1.0f,
                    .scalein  = 0.2f,
                    .scaleout = 0.8f,
                    .kind     = PARTICLE_ORB,
                    .color    = {1, 1, 1, 0.9f},
                    .ttl      = 0.5f,
                },
        },
    [EMITTERKIND_BURST_FIRE] =
        {
            .ttl   = 1.0f,
            .burst = 60,
            .particle =
                {
                    .kind         = PARTICLE_FIRE,
                    .ttl          = 1.0f,
                    .randScaleout = 1,
                    .scaleout     = 0.9f,
                    .scalein      = 0.1f,
                    .scale        = 0.2f,
                    .speed        = 1.0f,
                    .color        = {1.0f, 0.0f, .0f, .88f},
                    .randLife     = 1,
                },
        },
    [EMITTERKIND_BURST_SPARKS] =
        {
            .ttl   = 1.0f,
            .burst = 100,
            .particle =
                {
                    .kind         = PARTICLE_SHOCK_ADD,
                    .ttl          = 0.33f,
                    .randScaleout = 1,
                    .scaleout     = 0.9f,
                    .scalein      = 0.1f,
                    .scale        = 0.33f,
                    .speed        = 13.33f,
                    .color        = {1.0f, 1.0f, 1.0f, .88f},
                    .randLife     = 1,
                },
        },

    [EMITTERKIND_FOUNTAIN_FIRE] =
        {
            .inf       = 1,
            .rate      = 0.01f,
            .rateBurst = 2,
            .particle =
                {
                    .kind         = PARTICLE_FIRE,
                    .randLife     = 1,
                    .ttl          = 1.0f,
                    .randScaleout = 1,
                    .scaleout     = 0.9f,
                    .scalein      = 0.1f,
                    .offset       = 0.2f,
                    .scale        = 0.2f,
                    .speed        = 2.0f,
                    .color        = {1.0f, 0.0f, .0f, .88f},
                },
        },
    [EMITTERKIND_FOUNTAIN_FOG] =
        {
            .inf       = 1,
            .rate      = 0.05f,
            .rateBurst = 2,
            .particle =
                {
                    .kind         = PARTICLE_CLOUD,
                    .randLife     = 1,
                    .ttl          = 2.0f,
                    .randScaleout = 1,
                    .scaleout     = 0.9f,
                    .offset       = 0.2f,
                    .scalein      = 0.1f,
                    .scale        = 0.5f,
                    .speed        = 1.5f,
                    .color        = {1.0f, 1.0f, 1.0f, .66f},
                },
        },
    [EMITTERKIND_FOUNTAIN_SPARKS] =
        {
            .inf       = 1,
            .rate      = 0.05f,
            .rateBurst = 25,
            .particle  = {.randScale = 1,
                          .ttl       = 0.5f,
                          .offset    = 0.2f,
                          .scale     = 0.15f,
                          .color     = {1.0, .5f, .0f, .7f},
                          .kind      = PARTICLE_SHOCK_ADD,
                          .speed     = 3.0f,
                          .scalein   = 0.1f,
                          .scaleout  = 0.3f},
        },
};
