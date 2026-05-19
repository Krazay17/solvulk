#include "sol_core.h"

void Fx_Event(World *world, double dt, double time);

void Sol_View_Fx(World *world)
{
    WAddStep(world) = Fx_Event;
}

void Fx_Event(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e = &world->events->event[i];
        if (e->kind != EVENT_FX)
            continue;
        vec3s pos   = e->as.fx.pos;
        vec3s rot   = e->as.fx.rot;
        float scale = e->as.fx.scale > 0 ? e->as.fx.scale : 0.2f;
        switch (e->as.fx.kind)
        {
        case FXKIND_FIREBALL_HIT:
            Sol_Emitter_Add(world, (Emitter){.burst    = 200,
                                             .pos      = pos,
                                             .particle = {.randScale = 1,
                                                          .ttl       = 0.35f,
                                                          .scale     = 0.4f,
                                                          .color     = {0, 1, 0, .5f},
                                                          .kind      = PARTICLE_SPIKEY,
                                                          .speed     = 15.0f,
                                                          .scalein   = 0.1f,
                                                          .scaleout  = 0.3f}});
            Sol_Emitter_Add(world, (Emitter){.burst    = 1,
                                             .pos      = pos,
                                             .particle = {.scale   = 4.0f,
                                                          .scalein = 1.0f,
                                                          .kind    = PARTICLE_ORB,
                                                          .color   = (vec4s){0, 1, 0, 0.9f},
                                                          .ttl     = 0.2f}});
            Sol_Emitter_Add(world, (Emitter){.burst    = 1,
                                             .pos      = pos,
                                             .particle = {.scale    = 3.0f,
                                                          .scalein  = .5f,
                                                          .scaleout = .5f,
                                                          .kind     = PARTICLE_ORB,
                                                          .color    = (vec4s){1, 1, 1, 0.9f},
                                                          .ttl      = 0.2f}});
            break;
        case FXKIND_FIRE_APPLY:
            Sol_Emitter_Add(world, (Emitter){.burst    = 80,
                                             .pos      = pos,
                                             .particle = {.randScale = 1,
                                                          .ttl       = 1.0f,
                                                          .scale     = 0.4f,
                                                          .color     = {1, 1, 1, 1},
                                                          .kind      = PARTICLE_GFLAME,
                                                          .speed     = 2.0f,
                                                          .scalein   = 0.2f,
                                                          .scaleout  = 0.8f}});
            break;
        case FXKIND_SHIELD_BURST:
            Sol_Emitter_Add(world, (Emitter){.burst    = 1,
                                             .pos      = pos,
                                             .particle = {.scale    = 6.0f,
                                                          .scalein  = 1.0f,
                                                          .kind     = PARTICLE_ORB,
                                                          .color    = (vec4s){0.25f, 0.1f, 0.5f, 0.6f},
                                                          .ttl      = 0.2f,
                                                          .speed    = 0,
                                                          .followId = e->sourceId}});
            Sol_Emitter_Add(world, (Emitter){.burst    = 100,
                                             .rate     = 0.01f,
                                             .ttl      = 0.2f,
                                             .pos      = pos,
                                             .followId = e->sourceId,
                                             .particle = {.speed     = 8.0f,
                                                          .ttl       = 0.5f,
                                                          .scale     = 0.3f,
                                                          .randScale = 1,
                                                          .kind      = PARTICLE_SPIKEY,
                                                          .color     = (vec4s){0.25f, 0.1f, 0.5f, 0.6f},
                                                          .rot       = Sol_RandRange2(-2.0f, 2.0f),
                                                          .rotspeed  = 6.0f,
                                                          .followId  = e->sourceId,
                                                          .scaleout  = 0.2f,
                                                          .scalein   = 0.2f}});
            break;
        case FXKIND_SHIELD_HIT:
            Sol_Emitter_Add(world, (Emitter){.pos      = pos,
                                             .burst    = 2,
                                             .particle = {.ttl      = .1f,
                                                          .scale    = 1.0f,
                                                          .scaleout = 0.8f,
                                                          .kind     = PARTICLE_ORB,
                                                          .color    = (vec4s){0.25f, 0.1f, 0.5f, 1.0f},
                                                          .followId = e->targetId}});
            Sol_Emitter_Add(world, (Emitter){.burst    = 100,
                                             .rate     = 0.01f,
                                             .ttl      = 0.2f,
                                             .pos      = pos,
                                             .followId = e->targetId,
                                             .particle = {.speed     = 8.0f,
                                                          .ttl       = 0.5f,
                                                          .scale     = 0.3f,
                                                          .randScale = 1,
                                                          .kind      = PARTICLE_SPIKEY,
                                                          .color     = (vec4s){0.25f, 0.1f, 1.0f, 1.0f},
                                                          .rot       = Sol_RandRange2(-2.0f, 2.0f),
                                                          .rotspeed  = 6.0f,
                                                          .scaleout  = 0.2f,
                                                          .scalein   = 0.2f}});

            break;
        }
    }
}
