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
};

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

void Sol_Emitter_Init(World *world)
{
    world->tickSystems[world->tickCount++] = Emitter_Tick;

    // TODO MOVE TO CLIENT INIT
    world->draw3dSystems[world->draw3dCount++] = Sol_View_Particle_Draw;

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

    if (em->particle.ttl == 0)
        em->particle.ttl = 0.5f;
    if (em->particle.scale == 0)
        em->particle.scale = 0.5f;

    for (int i = 0; i < em->burst; i++)
    {
        Particle *p = Particle_Activate(s, em);
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

        if (!e->inf)
        {
            e->ttl -= fdt;
            if (e->ttl <= 0)
                continue;
        }

        if (e->followId)
            e->pos = Sol_Xform_GetPos(world, e->followId);
        else
            e->pos = vecAdd(e->pos, vecSca(e->vel, fdt));

        // Emitter spawn particle over time
        e->accumulator += fdt;
        if (e->rate > 0)
            // substep spawn incase lag to spawn right amount
            while (e->accumulator >= e->rate)
            {
                Particle *p = Particle_Activate(sys, e);
                // Rewind lag to place particle unclumped
                float lagOffset = e->accumulator / fdt;
                p->pos          = vecAdd(p->pos, vecSca(p->vel, -lagOffset * fdt));
                e->accumulator -= e->rate;
            }

        sys->emitter[write++] = *e;
    }
    world->emitters->emitter_count = write;

    Particle_Tick(world, dt, time);
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

        if (p->ttl < p->span)
        {
            vec3s finalvel = p->vel;
            if (p->followId)
                finalvel = vecAdd(p->vel, Sol_Physx_GetVel(world, p->followId));

            p->pos = vecAdd(p->pos, vecSca(finalvel, fdt));

            p->rot += p->rotspeed * dt;
        }

        s->particle[write++] = *p;
    }
    s->particle_count = write;
}

static Particle *Particle_Activate(SolEmitters *s, Emitter *e)
{
    Sol_Realloc(&s->particle, s->particle_count, &s->particle_capacity, sizeof(Particle));

    switch (e->particle.kind) {}

    Particle *p = &s->particle[s->particle_count++];
    memcpy(p, &e->particle, sizeof(Particle));
    p->scaleout = p->randScaleout ? (rand() % 100) * p->scaleout * 0.01f : p->scaleout;
    p->ttl      = p->randLife ? (rand() % 100) * p->ttl * 0.01f : p->ttl;
    p->span     = p->ttl;
    p->ttl += p->delay;

    p->scale    = p->randScale ? (rand() % 100) * p->scale * 0.01f : p->scale;
    float theta = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;    // 0 to 2pi
    float phi   = acosf(2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f); // 0 to pi

    float speed = p->speed;

    p->vel.x = sinf(phi) * cosf(theta) * speed;
    p->vel.y = sinf(phi) * sinf(theta) * speed;
    p->vel.z = cosf(phi) * speed;
    p->pos   = vecAdd(e->pos, vecSca(p->vel, p->offset));

    Sol_Debug_Add("Particles", (float)s->particle_count);
    return p;
}

u32 Sol_Emitter_GetParticleCount(World *world)
{
    return world->emitters->particle_count;
}

Particle *Sol_Emitter_GetParticles(World *world)
{
    return world->emitters->particle;
}