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
static void      Sol_Emitter_Step(World *world, double dt, double time);

void Sol_Emitter_Init(World *world)
{
    WAddStep(world)     = Sol_Emitter_Step;
    WAddTick(world)     = Emitter_Tick;
    WAdd3d(world)       = Sol_View_Particle_Draw;
    world->emitters     = malloc(sizeof(SolEmitters));
    world->compEmitters = calloc(MAX_ENTS, sizeof(CompEmitter));

    world->emitters->emitter_count    = 0;
    world->emitters->emitter_capacity = MAX_EMITTERS;
    world->emitters->emitter          = calloc(world->emitters->emitter_capacity, sizeof(Emitter));

    world->emitters->particle_count    = 0;
    world->emitters->particle_capacity = MAX_PARTICLES;
    world->emitters->particle          = calloc(world->emitters->particle_capacity, sizeof(Particle));
}

void Sol_Emitter_Add(World *world, int id, EmitterKind kind, float scale)
{
    CompEmitter *e = &world->compEmitters[id];
    if (!(world->masks[id] & HAS_EMITTER))
        e->emitterCount = 0;
    if (e->emitterCount >= MAX_COMPEMITTERS)
        return;

    Emitter *em = &e->emitters[e->emitterCount++];
    *em         = emitter_kinds[kind];

    em->particle.offset *= scale;
    em->particle.scale *= scale;

    world->masks[id] |= HAS_EMITTER;
}

static void Sol_Emitter_Step(World *world, double dt, double time)
{
    float        fdt = (float)dt;
    SolEmitters *sys = world->emitters;

    int required = HAS_EMITTER;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompEmitter *emitterComp = &world->compEmitters[id];
        int          write       = 0;
        for (int b = 0; b < emitterComp->emitterCount; b++)
        {
            Emitter *e = &emitterComp->emitters[b];
            if (!e->inf)
            {
                e->ttl -= fdt;
                if (e->ttl <= 0)
                    continue;
            }
            e->pos = Sol_Xform_GetPos(world, id);

            e->accumulator += fdt;
            if (e->rate > 0)
            {
                while (e->accumulator >= e->rate)
                {
                    for (int b = 0; b < e->rateBurst || b < 1; b++)
                    {
                        Particle *p = Particle_Activate(sys, e);
                        // Rewind lag to place particle unclumped
                        float lagOffset = e->accumulator / fdt;
                        p->pos          = vecAdd(p->pos, vecSca(p->vel, -lagOffset * fdt));
                    }
                    e->accumulator -= e->rate;
                }
            }
            emitterComp->emitters[write++] = *e;
        }
        emitterComp->emitterCount = write;

        if (write == 0)
            world->masks[id] &= ~HAS_EMITTER;
    }
}

void Sol_Emitter_Spawn(World *world, EmitterKind kind, vec3s pos, vec4s color, float scale)
{
    SolEmitters *s = world->emitters;
    Sol_Realloc(&s->emitter, s->emitter_count, &s->emitter_capacity, sizeof(Emitter));
    Emitter e         = emitter_kinds[kind];
    e.pos             = pos;
    e.particle.scale  = e.particle.scale * scale;
    e.particle.offset = e.particle.offset * scale;
    e.particle.color  = color;
    for (int i = 0; i < e.burst; i++)
        Particle_Activate(s, &e);
    if (e.rate > 0)
        s->emitter[s->emitter_count++] = e;
}

void Sol_Emitter_SpawnEx(World *world, Emitter e)
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
                for (int b = 0; b < e->rateBurst || b < 1; b++)
                {
                    Particle *p = Particle_Activate(sys, e);
                    // Rewind lag to place particle unclumped
                    float lagOffset = e->accumulator / fdt;
                    p->pos          = vecAdd(p->pos, vecSca(p->vel, -lagOffset * fdt));
                }
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

    // switch (e->particle.kind) {}

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

typedef enum
{
    PRENDER_SPHERE,
    PRENDER_FIREBALL,
    PRENDER_QUAD,
} ParticleRenderKind;
static const struct
{
    ParticleRenderKind render;
    u32                quadkind;
    u32                textureId;
} particle_render[PARTICLE_COUNT] = {
    [PARTICLE_ORB]      = {.render = PRENDER_SPHERE},
    [PARTICLE_FIREBALL] = {.render = PRENDER_FIREBALL},
    [PARTICLE_FIRE]     = {.render = PRENDER_QUAD, .quadkind = QUADKIND_SPRITE, .textureId = SOL_TEXTURE_FIREPARTICLE},
    [PARTICLE_SHOCK]    = {.render    = PRENDER_QUAD,
                           .quadkind  = QUADKIND_SPRITE_ADD,
                           .textureId = SOL_TEXTURE_SHOCKPARTICLE},
    [PARTICLE_CLOUD]    = {.render    = PRENDER_QUAD,
                           .quadkind  = QUADKIND_SPRITE_ADD,
                           .textureId = SOL_TEXTURE_CLOUDPARTICLE},
    [PARTICLE_BLOOD]    = {.render    = PRENDER_QUAD,
                           .quadkind  = QUADKIND_SPRITE_ADD,
                           .textureId = SOL_TEXTURE_BLOODPARTICLE},
};

void Sol_View_Particle_Draw(World *world, double dt, double time)
{
    u32 count = Sol_Emitter_GetParticleCount(world);
    if (count == 0)
        return;
    Particle *particles = Sol_Emitter_GetParticles(world);

    for (int i = 0; i < count; i++)
    {
        Particle *p = &particles[i];

        if (p->ttl > p->span)
            continue;

        float t           = p->ttl / p->span;
        float visualScale = p->scale;
        float visualAlpha = p->color.a;

        if ((1.0f - t) < p->scalein)
            visualScale = p->scale * ((1.0f - t) / p->scalein);
        if (t < p->scaleout)
            visualScale = p->scale * (t / p->scaleout);
        if ((1.0f - t) < p->fadein)
            visualAlpha = p->color.a * ((1.0f - t) / p->fadein);
        if (t < p->fadeout)
            visualAlpha = p->color.a * (t / p->fadeout);

        vec4s color = p->color.a > 0 ? p->color : (vec4s){1, 1, 1, 1};

        switch (p->kind)
        {
        case PARTICLE_ORB: {
            SphereSSBO *o = Sol_Render_GetNext_Sphere(true);
            o->pos        = (vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale};
            o->color      = color;
        }
        break;

        case PARTICLE_FIREBALL: {
            SphereSSBO *o = Sol_Render_GetNext_Fireball();
            o->pos        = (vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale};
            o->color      = color;
        }
        break;

        default: {
            // Everything else is a quad — pick texture + blend, then run shared setup.
            u32 textureId, quadKind;
            switch (p->kind)
            {
            case PARTICLE_BLOOD:
                textureId = SOL_TEXTURE_BLOODPARTICLE;
                quadKind  = QUADKIND_SPRITE;
                break;
            case PARTICLE_CLOUD:
                textureId = SOL_TEXTURE_CLOUDPARTICLE;
                quadKind  = QUADKIND_SPRITE_ADD;
                break;
            case PARTICLE_SHOCK:
                textureId = SOL_TEXTURE_SHOCKPARTICLE;
                quadKind  = QUADKIND_SPRITE_ADD;
                break;
            case PARTICLE_FIRE:
            default:
                textureId = SOL_TEXTURE_FIREPARTICLE;
                quadKind  = QUADKIND_SPRITE_ADD;
                break;
            }

            QuadSSBO *o  = Sol_Render_GetNext_Quad(quadKind);
            o->pos       = (vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale};
            o->color     = color;
            o->uv        = (vec4s){0, 0, 1, 1};
            o->rot       = (vec4s){p->rot, 0, 0, 1};
            o->textureId = textureId;
        }
        break;
        }
    }
}
