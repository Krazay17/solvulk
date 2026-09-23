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
    Emitter *e1    = Sol_Emitter_Next(world, EMITTERKIND_SPHERE);
    e1->pos        = event.as.hit.pos;
    e1->p_color    = (vec4s){1, 1, 1, 1};
    e1->p_lifespan = 0.3f;
    e1->p_scale    = event.as.hit.power * 2.0f;
    e1->p_kind     = PARTICLE_PLASMA;

    // Emitter *e3    = Sol_Emitter_Next(world, EMITTERKIND_SPHERE);
    // e3->pos        = event.as.fx.pos;
    // e3->p_color    = VEC4_WHITE;
    // e3->p_lifespan = 0.3f;
    // e3->p_scale    = event.as.fx.scale;
    // e3->scale_curve = CURVE_LATEPULSE;

    Emitter *e2 = Sol_Emitter_Next(world, EMITTERKIND_SMOKE_BURST);
    e2->pos     = event.as.hit.pos;
    e2->p_color = (vec4s){0.7f, 0.6f, 0.6f, 0.8f};

    if (world->doesRender)
        Sol_Audio_PlayAt(SOL_AUDIO_FIREBALLIMPACT, event.as.hit.pos, 1.0f, 0.0f, 16);
}

static inline void Fireball_Hit(World *world, SolEvent event)
{
    Emitter *e1 = Sol_Emitter_Next(world, EMITTERKIND_SPHERE);
    e1->pos     = event.as.hit.pos;
    e1->p_scale = 1.0f;

    Emitter *e2    = Sol_Emitter_Next(world, EMITTERKIND_BURST);
    e2->pos        = event.as.hit.pos;
    e2->p_kind     = PARTICLE_BLOOD;
    e2->p_color    = (vec4s){1, 0, 0, 1};
    e2->ttl        = 0;
    e2->p_lifespan = 0.5f;

    // Sol_Audio_PlayAt(SOL_AUDIO_LASER, event.as.fx.pos, 1.0f, 0.0f, 16);
}

static inline void Claw_Hit(World *world, SolEvent event)
{
    vec3s pos = event.as.hit.pos;

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

void Fx_Update(World *world, double dt)
{
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
            case HITKIND_BULLET:
            case HITKIND_MELEE_HIT:
                Claw_Hit(world, event);
                break;
            }
            break;
        case EVENTKIND_DEATH:
            break;
        default:
            sollog("No Hit event Fx handler", event.kind);
        }

        // if (event.kind != EVENTKIND_FX)
        //     continue;
        // switch (event.as.fx.kind)
        // {
        // case EVENTFX_FIREBALL_EXPLODE:
        //     Fireball_Explode(world, event);
        //     break;
        // case EVENTFX_FIREBALL_HIT:
        //     Fireball_Hit(world, event);
        //     break;
        // case EVENTFX_CLAW_HIT:
        //     Claw_Hit(world, event);
        //     break;
        // }
    }
}