/*
 * File: s_fx.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-22
 *
 */
#include "world.h"
#include "sol_math.h"
#include "audio.h"
#include "sol_user.h"

static inline void Fireball_Explode(World *world, SolEvent event)
{
    vec3s pos      = event.as.hit.pos;
    Emitter *e1    = Sol_Emitter_Next(world, EMITTERKIND_SPHERE);
    e1->pos        = pos;
    e1->p_color    = (vec4s){1.0f, 0.3f, 0.0f, 1.0f};
    e1->p_lifespan = 0.3f;
    e1->p_scale    = event.as.hit.power * 2.0f;
    e1->p_kind     = PARTICLE_SPHERE;

    Sol_Emitter_Spawn(world, EMITTERKIND_BURST_FIRE, pos);

    Emitter *e2 = Sol_Emitter_Next(world, EMITTERKIND_SMOKE_BURST);
    e2->pos     = pos;
    e2->p_color = (vec4s){0.7f, 0.6f, 0.6f, 0.8f};

    if (world->doesRender)
        Sol_Audio_PlayAt(SOL_AUDIO_FIREBALLIMPACT, pos, 1.0f, 0.0f, 16);
}
static inline void Fireball_Hit(World *world, SolEvent event)
{
    Emitter *e1    = Sol_Emitter_Next(world, EMITTERKIND_SPHERE);
    e1->p_color    = (vec4s){1, 0.3f, 0, 1};
    e1->p_lifespan = 0.3f;
    e1->pos        = event.as.hit.pos;
    e1->p_scale    = 1.0f;

    Emitter *e2    = Sol_Emitter_Next(world, EMITTERKIND_BURST);
    e2->pos        = event.as.hit.pos;
    e2->p_kind     = PARTICLE_BLOOD;
    e2->p_color    = (vec4s){1, 0, 0, 1};
    e2->ttl        = 0;
    e2->p_lifespan = 0.5f;
}
static inline void Fire_Hit(World *world, SolEvent event)
{
    Emitter *e     = Sol_Emitter_Next(world, EMITTERKIND_BURST);
    e->pos         = event.as.hit.pos;
    e->p_color     = (vec4s){1, 0.3f, 0, 1};
    e->p_lifespan  = 0.3f;
    e->p_scale     = 0.5f;
    e->burst       = 10;
    e->alpha_curve = CURVE_QUICKIN_SLOWOUT;
    e->scale_curve = CURVE_QUICKIN_SLOWOUT;
    e->speed       = 2.5f;
    e->p_kind      = PARTICLE_FIRE;
}
static inline void Claw_Hit(World *world, SolEvent event)
{
    vec3s pos   = event.as.hit.pos;
    Emitter *e1 = Sol_Emitter_Next(world, EMITTERKIND_SMOKE_BURST);
    e1->pos     = pos;

    Emitter *e2 = Sol_Emitter_Next(world, EMITTERKIND_BURST);
    e2->pos     = pos;
    e2->p_kind  = PARTICLE_SPARK;

    Emitter *e3    = Sol_Emitter_Next(world, EMITTERKIND_BURST);
    e3->pos        = pos;
    e3->p_kind     = PARTICLE_BLOOD;
    e3->p_color    = (vec4s){1, 0, 0, 1};
    e3->p_lifespan = 5.0f;

    if (world->doesRender)
        Sol_Audio_PlayAt(SOL_AUDIO_SWORDHIT, pos, 1.0f, 0.0f, 16);
}
static inline void Melee_Hit(World *world, SolEvent event)
{
    vec3s pos = event.as.hit.pos;

    Emitter *e2 = Sol_Emitter_Next(world, EMITTERKIND_BURST);
    e2->pos     = pos;
    e2->p_kind  = PARTICLE_SPARK;
    e2->p_scale = 0.4f;

    Emitter *e3 = Sol_Emitter_Next(world, EMITTERKIND_BURST);
    e3->pos     = pos;
    e3->burst   = 5;
    e3->p_kind  = PARTICLE_BLOOD;
    e3->p_color = (vec4s){1, 0, 0, 1};
    e3->p_scale = 0.4f;

    if (world->doesRender)
        Sol_Audio_PlayAt(SOL_AUDIO_SWORDHIT, pos, 1.0f, 0.0f, 16);
}
static inline void Normal_Hit(World *world, SolEvent event)
{
    vec3s pos = event.as.hit.pos;

    Emitter *e1 = Sol_Emitter_Next(world, EMITTERKIND_SMOKE_BURST);
    e1->pos     = event.as.hit.pos;
    e1->p_color = (vec4s){0.7f, 0.6f, 0.6f, 0.8f};

    Emitter *e2 = Sol_Emitter_Next(world, EMITTERKIND_BURST);
    e2->pos     = pos;
    e2->p_kind  = PARTICLE_SPARK;
    e2->p_scale = 0.4f;

    if (world->doesRender)
        Sol_Audio_PlayAt(SOL_AUDIO_FUZZHIT, pos, 0.4f, 0, 16);
}
static inline void Lightning_Hit(World *world, SolEvent event)
{
    vec3s pos   = event.as.hit.pos;
    Emitter *e1 = Sol_Emitter_Next(world, EMITTERKIND_SMOKE_BURST);
    e1->pos     = event.as.hit.pos;
    e1->p_color = (vec4s){0.7f, 0.6f, 0.6f, 0.8f};

    Emitter *e2 = Sol_Emitter_Next(world, EMITTERKIND_BURST);
    e2->pos     = pos;
    e2->p_kind  = PARTICLE_SPARK;
    e2->p_scale = 0.4f;
    Sol_Emitter_Spawn(world, EMITTERKIND_SHOCK_PULSE, pos);
    if (world->doesRender)
        Sol_Audio_PlayAt(SOL_AUDIO_LIGHTNINGHIT, pos, 0.6f, 0.05f, 16);
}

void Fx_Update(World *world, double dt)
{
    if (!world->doesRender)
        return;
    SlEvent *events = Sol_Comp_Get(world, 0, SlEvent);
    for (int i = 0; i < solb_count(events->events); i++)
    {
        SolEvent event = events->events[i];
        switch (event.kind)
        {
        case EVENTKIND_HIT:
            switch (event.as.hit.kind)
            {
            case HITKIND_FIREBALL_EXPLODE:
                Fireball_Explode(world, event);
                break;
            case HITKIND_FIREBALL:
                Fireball_Hit(world, event);
                break;
            case HITKIND_FIRE:
                Fire_Hit(world, event);
                break;
            case HITKIND_MELEE_HIT:
                Melee_Hit(world, event);
                break;
            case HITKIND_LIGHTNING:
                Lightning_Hit(world, event);
                break;
            default:
                Normal_Hit(world, event);
            }
            break;
        case EVENTKIND_FX:
            switch (event.as.fx.kind)
            {
            case FXKIND_INVULNHIT:
                Sol_Audio_PlayAt(SOL_AUDIO_WOONG, event.as.fx.pos, 0.8f, 0.2f, 16);
                break;
            case FXKIND_PARRY:
                Sol_Audio_PlayAt(SOL_AUDIO_PARRY, event.as.fx.pos, 1.0f, 0, 16);
                break;
            case FXKIND_TEST:
                Sol_Audio_PlayAt(SOL_AUDIO_WOONG, event.as.fx.pos, 1.0f, 0, 16);
                break;
            case FXKIND_SHOOT:
                Sol_Audio_PlayAt(SOL_AUDIO_SPACEGUN, event.as.fx.pos, 1.0f, 0.1f, 16);
                break;
            case FXKIND_SWORDSWING:
                Sol_Audio_PlayAt(SOL_AUDIO_SWORDSWING, event.as.fx.pos, 1.0f, 0, 16);
                break;
            default:
                sollog("EVENTKIND_FX no fx kind");
            }
            break;
        }
    }
}