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

void Emitter_Add(World *world, EmitterDesc e)
{
    SolEmitters *sys = world->emitters;
    if (sys->emitter_count >= sys->emitter_capacity)
    {
        sys->emitter_capacity *= 2;
        sys->emitter = realloc(sys->emitter, sizeof(Emitter) * sys->emitter_capacity);
    }

    u32 index = sys->emitter_count++;

    Emitter *em = &sys->emitter[index];
    *em         = (Emitter){
        .pos  = e.pos,
        .ttl  = e.ttl,
        .rate = e.rate,
        .vel  = e.vel,
        .particle =
            (Particle){
                .color = e.color,
                .ttl   = e.pttl,
                .scale = 0.1f,
            },
    };

    Sol_Debug_Add("Emitters", sys->emitter_count);
    for (int i = 0; i < e.burst; i++)
    {
        Particle *p = Particle_Activate(sys, em);
        p->scale    = (rand() % 100) * 0.001f;
        p->vel      = (vec3s){((float)(rand() % 100) / 50.0f) - 1.0f, ((float)(rand() % 100) / 50.0f) - 1.0f,
                              ((float)(rand() % 100) / 50.0f) - 1.0f};
    }
}

void Emitter_Step(World *world, double dt, double time)
{
    SolEvents *s = world->events;
    if (!s)
        return;
    for (int i = 0; i < s->count; i++)
    {
        SolEvent *e = &s->event[i];
        if (e->kind == EVENT_COLLISION)
            Emitter_Add(world, (EmitterDesc){.pos = e->pos,.burst=5, .color= (vec4s){255, 50, 50, 255}, .pttl = 0.5f});
    }
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
        if (e->ttl < 0)
            continue;
        e->pos = vecAdd(e->pos, vecSca(e->vel, fdt));

        e->accumulator += fdt;
        while (e->accumulator >= e->rate)
        {
            Particle *p = Particle_Activate(sys, e);

            float offset = e->accumulator / fdt;
            p->pos       = vecAdd(e->pos, vecSca(e->vel, -offset * fdt));
            p->scale     = (rand() % 100) * 0.001f;
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
        Particle *p = &s->particle_pool[i];
        float t = p->ttl / p->startTtl;
        float visualScale = p->scale * t;
        Sol_Submit_Sphere((vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale}, p->color);
    }
}

Particle *Particle_Activate(SolEmitters *sys, Emitter *init)
{
    if (sys->particle_count >= sys->particle_capacity)
        return &sys->particle_pool[0];

    u32       index = sys->particle_count++;
    Particle *p     = &sys->particle_pool[index];

    memcpy(p, &init->particle, sizeof(Particle));
    p->startTtl = p->ttl;
    p->pos = init->pos;

    return p;
}
