/*
 * File: s_emitter.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Particle Emitter
 * SolEmitters.emitter and SolEmitters.particle reallocs careful!
 */
#include "sol_core.h"

#define MAX_EMITTERS 0xFFF
#define MAX_PARTICLES 0xFFFF

typedef struct SolEmitters
{
    Emitter *emitter;
    u32      emitter_count;
    u32      emitter_capacity;

    Particle *particle;
    u32       particle_count;
    u32       particle_capacity;
} SolEmitters;

static Particle *Particle_Activate(SolEmitters *s, Emitter *init);
static void      Particle_Tick(World *world, double dt, double time);
static void      Emitter_Tick(World *world, double dt, double time);
static void      Emitter_Draw(World *world, double dt, double time);

void Sol_Emitter_Init(World *world)
{
    world->tickSystems[world->tickCount++]     = Emitter_Tick;
    world->draw3dSystems[world->draw3dCount++] = Emitter_Draw;

    world->emitters = malloc(sizeof(SolEmitters));

    world->emitters->emitter_count    = 0;
    world->emitters->emitter_capacity = MAX_EMITTERS;
    world->emitters->emitter          = calloc(world->emitters->emitter_capacity, sizeof(Emitter));

    world->emitters->particle_count    = 0;
    world->emitters->particle_capacity = MAX_PARTICLES;
    world->emitters->particle          = calloc(world->emitters->particle_capacity, sizeof(Particle));
}

void Sol_Emitter_Add(World *world, Emitter e)
{
    SolEmitters *s = world->emitters;
    Sol_Realloc(&s->emitter, s->emitter_count, &s->emitter_capacity, sizeof(Emitter));

    Emitter *em = &s->emitter[s->emitter_count];
    *em         = e;

    for (int i = 0; i < em->burst; i++)
    {
        Particle *p = Particle_Activate(s, em);
        p->scale    = (rand() % 100) * p->scale * 0.01f;
        float theta = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;    // 0 to 2pi
        float phi   = acosf(2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f); // 0 to pi

        float speed = p->speed;

        p->vel.x = sinf(phi) * cosf(theta) * speed;
        p->vel.y = sinf(phi) * sinf(theta) * speed;
        p->vel.z = cosf(phi) * speed;
    }
    if (e.rate > 0)
        s->emitter_count++;
}

static void Emitter_Tick(World *world, double dt, double time)
{
    if (!world->emitters)
        return;

    float        fdt = (float)dt;
    SolEmitters *sys = world->emitters;

    int write = 0;
    for (int i = 0; i < sys->emitter_count; i++)
    {
        Emitter *e = &sys->emitter[i];
        e->ttl -= fdt;
        if (e->ttl <= 0)
            continue;

        e->pos = vecAdd(e->pos, vecSca(e->vel, fdt));

        // Emitter spawn particle over time
        e->accumulator += fdt;
        if (e->rate > 0)
            // substep spawn incase lag to spawn right amount
            while (e->accumulator >= e->rate)
            {
                Particle *p = Particle_Activate(sys, e);
                // Rewind lag to place particle unclumped
                float offset = e->accumulator / fdt;
                p->pos       = vecAdd(e->pos, vecSca(e->vel, -offset * fdt));
                p->scale     = (rand() % 100) * p->scale * 0.01f;
                // Random spherical velocity
                float theta = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;    // 0 to 2pi
                float phi   = acosf(2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f); // 0 to pi

                float speed = p->speed > 0 ? p->speed : 1.0f;

                p->vel.x = sinf(phi) * cosf(theta) * speed;
                p->vel.y = sinf(phi) * sinf(theta) * speed;
                p->vel.z = cosf(phi) * speed;

                e->accumulator -= e->rate;
            }

        sys->emitter[write++] = *e;
    }
    world->emitters->emitter_count = write;

    Particle_Tick(world, dt, time);
}

static void Emitter_Draw(World *world, double dt, double time)
{
    float        fdt = (float)dt;
    SolEmitters *s   = world->emitters;
    for (int i = 0; i < s->particle_count; i++)
    {
        Particle *p           = &s->particle[i];
        float     t           = p->ttl / p->span;
        float     visualScale = p->scale * t;
        vec4s     color       = p->color.a > 0 ? p->color : (vec4s){1, 1, 1, 1};

        switch (p->kind)
        {
        case PARTICLE_ORB:
            Sol_Render_PushSphere((SphereDesc){
                .pos   = (vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale},
                .color = color,
            });
            break;
        case PARTICLE_GFLAME:
            Sol_Render_PushQuad((QuadDesc){
                .pos      = (vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale},
                .color    = color,
                .rotation = (versors){p->rot, 0, 0, 1.0f},
            });
            break;
        default:
            Sol_Render_PushSphere((SphereDesc){
                .pos   = (vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale},
                .color = color,
            });
        }
    }
}

static void Particle_Tick(World *world, double dt, double time)
{
    float        fdt   = (float)dt;
    SolEmitters *s     = world->emitters;
    int          write = 0;

    for (int i = 0; i < s->particle_count; i++)
    {
        Particle *p = &s->particle[i];
        p->ttl -= fdt;
        if (p->ttl <= 0)
            continue;
        p->pos = vecAdd(p->pos, vecSca(p->vel, fdt));
        p->rot += p->rotspeed * dt;

        s->particle[write++] = *p;
    }
    s->particle_count = write;
}

static Particle *Particle_Activate(SolEmitters *s, Emitter *init)
{
    Sol_Realloc(&s->particle, s->particle_count, &s->particle_capacity, sizeof(Particle));

    Particle *p = &s->particle[s->particle_count];
    memcpy(p, &init->particle, sizeof(Particle));
    p->span = p->ttl;
    p->pos  = init->pos;

    s->particle_count++;

    Sol_Debug_Add("Particles", s->particle_count);
    return p;
}
