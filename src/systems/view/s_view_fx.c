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
        if (e->kind != EVENTKIND_FX)
            continue;
        vec3s pos   = e->as.fx.pos;
        vec3s rot   = e->as.fx.rot;
        float scale = e->as.fx.scale > 0 ? e->as.fx.scale : 0.2f;

        switch (e->as.fx.kind)
        {
        case FXKIND_SWORD_SWING: {
            Sol_Audio_PlayAt(SOL_AUDIO_SWORD_SWING, e->as.fx.pos, 0.5f, 0.0f, 16);
        }
        break;
        case FXKIND_SWORD_HIT: {
            Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, e->as.fx.pos, (vec4s){1,1,1,1}, 0.5f);
            Sol_Audio_PlayAt(SOL_AUDIO_SWORDHIT, e->as.fx.pos, 1.0f, 0, 16);
        }
        break;
        case FXKIND_BULLET_HIT: {
            Sol_Emitter_Spawn(world, EMITTERKIND_FLASH_BALL, e->as.fx.pos, (vec4s){1,0,0,1},0.25f);
            Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, e->as.fx.pos, (vec4s){1,1,1,1},0.25f);
        }
        break;
        case FXKIND_SPINHIT: {
            Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, e->as.fx.pos,(vec4s){1,1,1,1}, 1.0f);
            Sol_Emitter_Spawn(world, EMITTERKIND_BURST_CLOUDS, e->as.fx.pos, (vec4s){1,1,1,1},1.0f);
        }
        break;
        case FXKIND_FIREBALL_SHOOT: {
            Sol_Audio_PlayAt(SOL_AUDIO_FIREBALL, e->as.fx.pos, 1.0f, 0.0f, 0);
            // Sol_Emitter_SpawnEx(world, (Emitter){.pos      = pos,
            //                                  .ttl      = 1.0f,
            //                                  .burst    = 1,
            //                                  .rate     = 0.1f,
            //                                  .followId = e->as.fx.entA,
            //                                  .particle = {
            //                                      .kind     = PARTICLE_CLOUD,
            //                                      .ttl      = 5.2f,
            //                                      .scaleout = 0.9f,
            //                                      .scalein  = 0.04f,
            //                                      .scale    = .2f * scale,
            //                                      .speed    = 2.0f,
            //                                      .color    = {.2f, .2f, .2f, .88f},
            //                                  }});
            // Sol_Emitter_SpawnEx(world, (Emitter){.pos      = pos,
            //                                  .ttl      = 2.0f,
            //                                  .burst    = 5,
            //                                  .rate     = 0.01f,
            //                                  .followId = e->as.fx.entA,
            //                                  .particle = {
            //                                      .kind     = PARTICLE_FIRE,
            //                                      .ttl      = 0.6f,
            //                                      .scaleout = 0.1f,
            //                                      .scalein  = 0.9f,
            //                                      .scale    = .2f * scale,
            //                                      .speed    = 2.0f,
            //                                      .color    = {.9f, 0, 0, .98f},
            //                                  }});
            // Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 10,
            //                                  .pos      = pos,
            //                                  .rate     = 0.02f,
            //                                  .ttl      = 2.0f,
            //                                  .followId = e->as.fx.entA,
            //                                  .particle = {.randScale = 1,
            //                                               .ttl       = 0.5f,
            //                                               .scale     = 0.5f * scale,
            //                                               .color     = {1.0, .5f, .0f, .7f},
            //                                               .kind      = PARTICLE_SHOCK,
            //                                               .speed     = 3.0f * scale,
            //                                               .scalein   = 0.1f,
            //                                               .scaleout  = 0.3f}});
        }
        break;
        case FXKIND_FIREBALL_HIT: {
            float scale = e->as.fx.scale;
            Sol_Audio_PlayAt(SOL_AUDIO_FIREBALLIMPACT, e->as.fx.pos, 1.0f, 0.05f, 0);

            // QUICK MAGIC BLAST
            Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 100,
                                                 .pos      = pos,
                                                 .particle = {.randScale = 1,
                                                              .ttl       = 0.3f,
                                                              .scale     = 0.5f * scale,
                                                              .color     = {1, .1f, .2f, .7f},
                                                              .kind      = PARTICLE_SHOCK,
                                                              .speed     = 15.0f * scale,
                                                              .scalein   = 0.1f,
                                                              .scaleout  = 0.3f}});
            // MAIN RED
            Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 1,
                                                 .pos      = pos,
                                                 .particle = {.scale    = 1.0f * scale,
                                                              .scalein  = 0.2f,
                                                              .scaleout = 0.8f,
                                                              .kind     = PARTICLE_ORB,
                                                              .color    = (vec4s){1, 0, 0, 0.9f},
                                                              .ttl      = 0.5f}});
            // DELAYED FIREBALL
            Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 1,
                                                 .pos      = pos,
                                                 .particle = {.scale    = 2.0f * scale,
                                                              .scalein  = 0.4f,
                                                              .scaleout = 0.6f,
                                                              .kind     = PARTICLE_FIREBALL,
                                                              .color    = (vec4s){1, 0, 0, 0.9f},
                                                              .delay    = 0.2f,
                                                              .ttl      = .16f}});
            // QUICK WHITE
            Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 1,
                                                 .pos      = pos,
                                                 .particle = {.scale    = 1.5f * scale,
                                                              .scalein  = .5f,
                                                              .scaleout = .5f,
                                                              .kind     = PARTICLE_ORB,
                                                              .color    = (vec4s){1.0f, 0.8f, 1.0f, 1.0f},
                                                              .ttl      = 0.3f}});
            // DUST
            Sol_Emitter_SpawnEx(world, (Emitter){.pos      = pos,
                                                 .ttl      = 1.0f,
                                                 .burst    = 40,
                                                 .particle = {
                                                     .kind     = PARTICLE_CLOUD,
                                                     .ttl      = 5.2f,
                                                     .scaleout = 0.9f,
                                                     .scalein  = 0.04f,
                                                     .scale    = 1.3f * scale,
                                                     .offset   = scale,
                                                     .speed    = 2.0f,
                                                     .color    = {.2f, .2f, .2f, .88f},
                                                 }});
            // FIRES
            Sol_Emitter_SpawnEx(world, (Emitter){.pos      = pos,
                                                 .burst    = 60,
                                                 .particle = {
                                                     .kind         = PARTICLE_FIRE,
                                                     .ttl          = 1.0f * scale,
                                                     .randScaleout = 1,
                                                     .scaleout     = 0.9f,
                                                     .scalein      = 0.1f,
                                                     .scale        = 0.35f * scale,
                                                     .offset       = scale,
                                                     .speed        = 1.0f,
                                                     .color        = {1.0f, 0.0f, .0f, .88f},
                                                     .randLife     = 1,
                                                 }});
            Sol_Emitter_SpawnEx(world, (Emitter){.pos       = pos,
                                                 .ttl       = 2.5f,
                                                 .burst     = 5,
                                                 .rate      = 0.2f,
                                                 .rateBurst = 2,
                                                 .particle  = {
                                                     .kind         = PARTICLE_FIRE,
                                                     .ttl          = 1.5f * scale,
                                                     .randScaleout = 1,
                                                     .scaleout     = 0.9f,
                                                     .scalein      = 0.1f,
                                                     .scale        = 0.35f * scale,
                                                     .offset       = scale,
                                                     .speed        = 0.5f,
                                                     .color        = {1.0f, 0.15f, .0f, .88f},
                                                     .randLife     = 1,
                                                 }});
            // SPARKS
            Sol_Emitter_SpawnEx(world, (Emitter){.pos      = pos,
                                                 .ttl      = 1.0f,
                                                 .burst    = 260,
                                                 .particle = {
                                                     .kind         = PARTICLE_SHOCK,
                                                     .ttl          = 1.5f * scale,
                                                     .randLife     = 1,
                                                     .randScaleout = 1,
                                                     .randScale    = 1,
                                                     .scaleout     = 0.9f,
                                                     .scalein      = 0.1f,
                                                     .scale        = 0.12f,
                                                     .offset       = scale * 2,
                                                     .speed        = 1.0f,
                                                     .color        = {1.0f, 1.0f, .0f, .88f},
                                                 }});
        }
        break;
        case FXKIND_FIRE_APPLY: {
            Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 40,
                                                 .pos      = pos,
                                                 .particle = {.randScale = 1,
                                                              .ttl       = 1.0f,
                                                              .scale     = 0.4f,
                                                              .color     = {1, 0, 0, 1},
                                                              .kind      = PARTICLE_FIRE,
                                                              .speed     = 5.0f,
                                                              .scalein   = 0.2f,
                                                              .scaleout  = 0.8f}});
        }
        break;
        case FXKIND_SHIELD_BURST: {
            Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 1,
                                                 .pos      = pos,
                                                 .particle = {.scale    = 4.0f,
                                                              .scalein  = 1.0f,
                                                              .kind     = PARTICLE_ORB,
                                                              .color    = (vec4s){0.25f, 0.1f, 0.5f, 0.6f},
                                                              .ttl      = 0.2f,
                                                              .speed    = 0,
                                                              .followId = e->as.fx.entA}});
            Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 100,
                                                 .rate     = 0.01f,
                                                 .ttl      = 0.2f,
                                                 .pos      = pos,
                                                 .followId = e->as.fx.entA,
                                                 .particle = {.speed     = 8.0f,
                                                              .ttl       = 0.5f,
                                                              .scale     = 0.3f,
                                                              .randScale = 1,
                                                              .kind      = PARTICLE_SHOCK,
                                                              .color     = (vec4s){0.25f, 0.1f, 0.5f, 0.6f},
                                                              .rot       = Sol_Math_RandRange2(-2.0f, 2.0f),
                                                              .rotspeed  = 6.0f,
                                                              .followId  = e->as.fx.entA,
                                                              .scaleout  = 0.2f,
                                                              .scalein   = 0.2f}});
        }
        break;
        case FXKIND_SHIELD_HIT: {
            Sol_Emitter_SpawnEx(world, (Emitter){.pos      = pos,
                                                 .burst    = 2,
                                                 .particle = {.ttl      = .1f,
                                                              .scale    = 1.0f,
                                                              .scaleout = 0.8f,
                                                              .kind     = PARTICLE_ORB,
                                                              .color    = (vec4s){0.25f, 0.1f, 0.5f, 1.0f},
                                                              .followId = e->as.fx.entB}});
            Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 100,
                                                 .rate     = 0.01f,
                                                 .ttl      = 0.2f,
                                                 .pos      = pos,
                                                 .followId = e->as.fx.entA,
                                                 .particle = {.speed     = 8.0f,
                                                              .ttl       = 0.5f,
                                                              .scale     = 0.3f,
                                                              .randScale = 1,
                                                              .kind      = PARTICLE_SHOCK,
                                                              .color     = (vec4s){0.25f, 0.1f, 1.0f, 1.0f},
                                                              .rot       = Sol_Math_RandRange2(-2.0f, 2.0f),
                                                              .rotspeed  = 6.0f,
                                                              .scaleout  = 0.2f,
                                                              .scalein   = 0.2f}});
        }
        break;
        case FXKIND_DEATH_BLOOD: {
            Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 40,
                                                 .pos      = e->as.fx.pos,
                                                 .particle = (Particle){.color = {.r = 1.0f, .g = 0, .b = 0, .a = 1.0f},
                                                                        .kind  = PARTICLE_BLOOD,
                                                                        .scale = 0.4f,
                                                                        .ttl   = 1.5f,
                                                                        .speed = 2.5f,
                                                                        .scaleout = .5f}});
        }
        break;
        }
    }
}
