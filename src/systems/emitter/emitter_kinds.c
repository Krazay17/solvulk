#include "emitter.h"

Emitter emitter_kinds[] = {
    [EMITTERKIND_FLASH_WHITEBALL] =
        {
            .burst = 100,
            .particle =
                {
                    .randScale = 1,
                    .ttl       = 0.3f,
                    .scale     = 0.5f,
                    .color     = {1, .1f, .2f, .7f},
                    .kind      = PARTICLE_SHOCK,
                    .speed     = 15.0f,
                    .scalein   = 0.1f,
                    .scaleout  = 0.3f,
                },
        },
    [EMITTERKIND_FLASH_REDBALL] =
        {
            .burst = 1,
            .particle =
                {
                    .scale    = 1.0f,
                    .scalein  = 0.2f,
                    .scaleout = 0.8f,
                    .kind     = PARTICLE_ORB,
                    .color    = {1, 0, 0, 0.9f},
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
};
