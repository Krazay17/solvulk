#include "s_event.h"
#include "world.h"
#include "audio.h"
#include "audio/s_audio.h"
#include "emitter/s_emitter.h"
#include "ribbon/s_ribbon.h"

typedef void (*FxHandler)(World *world, const SolEvent *e);

static const u32 hitSound_map[EKIND_COUNT] = {
    [EKIND_PLAYER] = SOL_AUDIO_GOTHIT,
    //    [EKIND_WIZARD] = SOL_AUDIO_GOTHIT,
};

static void Handle_FireballHit(World *world, const SolEvent *e)
{
    vec3s pos   = e->as.fx.pos;
    float scale = e->as.fx.scale;

    Sol_Audio_PlayAt(SOL_AUDIO_FIREBALLIMPACT, pos, 1.0f, 0.05f, 0);

    // QUICK MAGIC BLAST
    Sol_Emitter_SpawnEx(world, (Emitter){.burst    = 100,
                                         .pos      = pos,
                                         .particle = {.randScale = 1,
                                                      .ttl       = 0.3f,
                                                      .scale     = 0.5f * scale,
                                                      .color     = {1, .1f, .2f, .7f},
                                                      .kind      = PARTICLE_SHOCK_ADD,
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
                                             .kind         = PARTICLE_SHOCK_ADD,
                                             .ttl          = 1.5f * scale,
                                             .randLife     = 1,
                                             .randScaleout = 1,
                                             .randScale    = 1,
                                             .scaleout     = 0.9f,
                                             .scalein      = 0.1f,
                                             .scale        = 0.12f,
                                             .offset       = scale * 2.0f,
                                             .speed        = 1.0f,
                                             .color        = {1.0f, 1.0f, .0f, .88f},
                                         }});
}

static void Handle_FireballShoot(World *world, const SolEvent *e)
{
    Sol_Audio_PlayAt(SOL_AUDIO_FIREBALL, e->as.fx.pos, e->as.fx.scale, 0.0f, 0);
}

static void Handle_SwordSwing(World *world, const SolEvent *e)
{
    Sol_Audio_PlayAt(SOL_AUDIO_SWORD_SWING, e->as.fx.pos, e->as.fx.scale, 0.0f, 32);
}

static void Handle_LaserHit(World *world, const SolEvent *e)
{
    Sol_Emitter_SpawnEx(world, (Emitter){.pos      = e->as.fx.pos,
                                         .burst    = 25,
                                         .particle = {
                                             .kind    = PARTICLE_SHOCK,
                                             .scale   = 0.3f,
                                             .ttl     = 0.2f,
                                             .color   = (vec4s){0.0f, 1.0f, 0.0f, 1.0f},
                                             .fadeout = 1.0f,
                                             .speed   = 7.0f,
                                         }});
}

static void Handle_Lightning(World *world, const SolEvent *e)
{
    Sol_Emitter_Spawn(world, EMITTERKIND_SINGLE_SPARK, e->as.fx.pos, (vec4s){0.8f, 1.0f, 1.0f, 1.0f}, 1.0f);
}

static void Handle_TakeDamage(World *world, const SolEvent *e)
{
    u32 hitSoundId = hitSound_map[world->ekinds[e->as.fx.entB]];
    Sol_Audio_PlayAt(hitSoundId, e->as.fx.pos, e->as.fx.scale, 0.0f, 32);
}

static void Handle_ChainLightning(World *world, const SolEvent *e)
{
    u32   a   = e->as.fx.entA;
    u32   b   = e->as.fx.entB;
    vec3s pos = e->as.fx.pos;
    Sol_Audio_PlayAt(SOL_AUDIO_LIGHTNINGHIT, pos, 1.0f, 0, 64);
    Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, pos, (vec4s){0.5f, 0.5f, 1.0f, 0.8f}, 0.2f);
    Sol_Ribbon_AddBetweenEntities(world, a, b, RIBBONKIND_LIGHTNING, 1.0f, (vec4s){0.5f, 0.5f, 1.0f, 0.8f});
    Sol_Ribbon_AddBetweenEntities(world, a, b, RIBBONKIND_LIGHTNING, 0.4f, (vec4s){1.0f, 1.0f, 1.0f, 1.0f});
    Sol_Emitter_Add(world, a, EMITTERKIND_SINGLE_SPARK, (vec4s){0.5f, 0.5f, 1.0f, 0.8f}, 0.4f);
    Sol_Emitter_Add(world, a, EMITTERKIND_SINGLE_SPARK, (vec4s){1.0f, 1.0f, 1.0f, 0.2f}, 0.4f);
    Sol_Emitter_Add(world, b, EMITTERKIND_SINGLE_SPARK, (vec4s){0.5f, 0.5f, 1.0f, 0.8f}, 0.4f);
    Sol_Emitter_Add(world, b, EMITTERKIND_SINGLE_SPARK, (vec4s){1.0f, 1.0f, 1.0f, 0.2f}, 0.4f);
}

static const FxHandler fxHandler[FXKIND_COUNT] = {
    [FXKIND_FIREBALL_SHOOT] = Handle_FireballShoot, [FXKIND_FIREBALL_HIT] = Handle_FireballHit,
    [FXKIND_SWORD_SWING] = Handle_SwordSwing,       [FXKIND_LASER_HIT] = Handle_LaserHit,

    [FXKIND_FIRE_APPLY] = Handle_SwordSwing,        [FXKIND_SHIELD_BURST] = Handle_SwordSwing,
    [FXKIND_SHIELD_HIT] = Handle_SwordSwing,        [FXKIND_SPINHIT] = Handle_SwordSwing,
    [FXKIND_BULLET_HIT] = Handle_SwordSwing,        [FXKIND_CHAINLIGHTNING] = Handle_ChainLightning,
    [FXKIND_LIGHTNING] = Handle_Lightning,          [FXKIND_SWORD_HIT] = Handle_SwordSwing,
    [FXKIND_TAKEDAMAGE] = Handle_TakeDamage,        [FXKIND_DEATH_BLOOD] = Handle_SwordSwing,
};

static void Event_HandleFx(World *world, double dt, double time)
{
    SolEvents *events = world->events;
    for (int i = 0; i < events->count; i++)
    {
        SolEvent *e = &events->event[i];
        if (e->kind != EVENTKIND_FX)
            continue;
        if (e->as.fx.scale == 0.0f)
            e->as.fx.scale = 1.0f;
        FxHandler handler = fxHandler[e->as.fx.kind];
        if (handler)
            handler(world, e);
    }
}

void Sol_Event_HandleFx_Init(World *w)
{
    WAddStep(w) = Event_HandleFx;
}