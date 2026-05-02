#include "emitter.h"
#include "render/render.h"
#include "sol_core.h"

void Emitter_Init(World *world)
{
    world->emitters = malloc(sizeof(SolEmitters));

    world->emitters->emitter_count    = 0;
    world->emitters->emitter_capacity = MAX_EMITTERS;
    world->emitters->emitter          = calloc(world->emitters->emitter_capacity, sizeof(Emitter));

    world->emitters->particle_count    = 0;
    world->emitters->particle_capacity = MAX_PARTICLES;
    world->emitters->particle_pool     = calloc(world->emitters->particle_capacity, sizeof(Particle));
}

void Emitter_Add(World *world, Emitter e)
{
    SolEmitters *s = world->emitters;
    Sol_Realloc(&s->emitter, s->emitter_count, &s->emitter_capacity, sizeof(Emitter));

    Emitter *em = &s->emitter[s->emitter_count];
    *em = e;

    for (int i = 0; i < em->burst; i++)
    {
        Particle *p = Particle_Activate(s, em);
        p->scale    = (rand() % 100) * p->scale * 0.01f;
        p->vel      = (vec3s){((float)(rand() % 100) / 50.0f) - 1.0f, ((float)(rand() % 100) / 50.0f) - 1.0f,
                              ((float)(rand() % 100) / 50.0f) - 1.0f};
    }
    if (e.rate > 0)
        s->emitter_count++;
}

void Emitter_Step(World *world, double dt, double time)
{
}

void Emitter_Tick(World *world, double dt, double time)
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

        e->accumulator += fdt;
        if (e->rate > 0)
            while (e->accumulator >= e->rate)
            {
                Particle *p = Particle_Activate(sys, e);

                float offset = e->accumulator / fdt;
                p->pos       = vecAdd(e->pos, vecSca(e->vel, -offset * fdt));
                p->scale     = (rand() % 100) * p->scale * 0.01f;
                p->vel       = (vec3s){((float)(rand() % 100) / 50.0f) - 1.0f, ((float)(rand() % 100) / 50.0f) - 1.0f,
                                       ((float)(rand() % 100) / 50.0f) - 1.0f};

                e->accumulator -= e->rate;
            }

        sys->emitter[write++] = *e;
    }
    world->emitters->emitter_count = write;

    Particle_Tick(world, dt, time);
}

void Particle_Tick(World *world, double dt, double time)
{
    float        fdt   = (float)dt;
    SolEmitters *s     = world->emitters;
    int          write = 0;

    for (int i = 0; i < s->particle_count; i++)
    {
        Particle *p = &s->particle_pool[i];
        p->ttl -= fdt;
        if (p->ttl <= 0)
            continue;
        p->pos = vecAdd(p->pos, vecSca(p->vel, fdt));

        s->particle_pool[write++] = *p;
    }

    // The count is now exactly how many particles we 'wrote'
    s->particle_count = write;
}

void Emitter_Draw(World *world, double dt, double time)
{
    float        fdt = (float)dt;
    SolEmitters *s   = world->emitters;
    for (int i = 0; i < s->particle_count; i++)
    {
        Particle *p           = &s->particle_pool[i];
        float     t           = p->ttl / p->startTtl;
        float     visualScale = p->scale * t;
        Sol_Submit_Sphere((vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale}, p->color);
    }
}

Particle *Particle_Activate(SolEmitters *s, Emitter *init)
{
    Sol_Realloc(&s->particle_pool, s->particle_count, &s->particle_capacity, sizeof(Particle));

    Particle *p = &s->particle_pool[s->particle_count];
    memcpy(p, &init->particle, sizeof(Particle));
    p->startTtl = p->ttl;
    p->pos      = init->pos;

    s->particle_count++;

    Sol_Debug_Add("Particles", s->particle_count);
    return p;
}
