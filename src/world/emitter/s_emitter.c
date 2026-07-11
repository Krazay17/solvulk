/*
 * File: s_emitter.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Particle Emitter
 */
// #include "sol_core.h"
#include "s_emitter.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "event/s_event.h"
#include "render/render.h"
#include "xform/s_xform.h"
#include "physx/s_body.h"

#define MAX_EMITTERS 0xFFF
#define MAX_PARTICLES 0xFFFF

typedef struct EmitterQueue
{
    vec3s pos, rot, color;
    float scale;
    u8    kind, entA, entB;
} EmitterQueue;

typedef struct SolEmitters
{
    Emitter *emitter;
    u32      emitter_count;
    u32      emitter_capacity;

    Particle *particle;
    u32       particle_count;
    u32       particle_capacity;
} SolEmitters;

static Particle *Particle_Activate(SolEmitters *s, Emitter *e);
static void      Particle_Tick(World *world, double dt, double time);
static void      Particle_Draw(World *world, double dt, double time);
static void      Emitter_Step(World *world, double dt, double time);

void Sol_Make_Emitter(World *world, Emitter e)
{
    SolEmitters *s = world->emitters;
    for (int i = 0; i < e.burst; i++)
        Particle_Activate(s, &e);
    if (e.ttl > 0 || e.inf)
    {
        Sol_Realloc((void **)&s->emitter, s->emitter_count, &s->emitter_capacity, sizeof(Emitter));
        s->emitter[s->emitter_count++] = e;
    }
}

void Sol_Emitter_Init(World *world)
{
    world->emitters                    = malloc(sizeof(SolEmitters));
    world->emitters->emitter_count     = 0;
    world->emitters->emitter_capacity  = MAX_EMITTERS;
    world->emitters->emitter           = calloc(MAX_EMITTERS, sizeof(Emitter));
    world->emitters->particle_count    = 0;
    world->emitters->particle_capacity = MAX_PARTICLES;
    world->emitters->particle          = calloc(MAX_PARTICLES, sizeof(Particle));

    WAddStep(world) = Emitter_Step;
    WAddTick(world) = Particle_Tick;
    WAdd3d(world)   = Particle_Draw;
}

// --- public API ---

Emitter *Sol_Emitter_Spawn(World *world, EmitterKind kind, vec3s pos, vec4s color, float scale)
{
    SolEmitters *s = world->emitters;
    Sol_Realloc((void **)&s->emitter, s->emitter_count, &s->emitter_capacity, sizeof(Emitter));

    Emitter *e = &s->emitter[s->emitter_count++];
    *e         = emitter_kinds[kind];
    e->pos     = pos;
    e->particle.scale *= scale;
    e->particle.offset *= scale;
    if (color.a > 0)
        e->particle.color = color;

    for (int i = 0; i < e->burst; i++)
        Particle_Activate(s, e);

    // Burst-only emitters don't need to stay alive
    if (e->rate <= 0 && !e->inf)
    {
        s->emitter_count--;
        return NULL;
    }
    return e;
}

Emitter *Sol_Emitter_Add(World *world, int id, EmitterKind kind, vec4s color, float scale)
{
    Emitter *e = Sol_Emitter_Spawn(world, kind, Sol_Xform_GetPos(world, id), (vec4s){0}, scale);
    if (!e)
        return NULL;

    e->followId       = id;
    e->followIdGen    = Sol_GetEntGen(id);
    e->particle.color = color;
    return e;
}

void Sol_Emitter_SpawnMulti(World *world, const Emitter *emitters, int count, vec3s pos, vec4s color, float scale,
                            int id)
{
    for (int i = 0; i < count; i++)
    {
        Emitter e = emitters[i];
        e.pos     = pos;
        if (id)
        {
            e.followId          = id;
            e.particle.followId = id;
        }
        e.particle.scale *= scale;
        e.particle.offset *= scale;
        if (color.a > 0)
            e.particle.color = color;
        Sol_Make_Emitter(world, e);
    }
}

Emitter *Sol_Emitter_SpawnEx(World *world, Emitter src)
{
    SolEmitters *s = world->emitters;
    Sol_Realloc((void **)&s->emitter, s->emitter_count, &s->emitter_capacity, sizeof(Emitter));

    Emitter *e = &s->emitter[s->emitter_count++];
    *e         = src;
    if (e->particle.ttl == 0)
        e->particle.ttl = 0.5f;
    if (e->particle.scale == 0)
        e->particle.scale = 0.5f;

    for (int i = 0; i < e->burst; i++)
        Particle_Activate(s, e);

    if (e->rate <= 0 && !e->inf)
    {
        s->emitter_count--;
        return NULL;
    }
    return e;
}

u32 Sol_Emitter_GetParticleCount(World *world)
{
    return world->emitters->particle_count;
}
Particle *Sol_Emitter_GetParticles(World *world)
{
    return world->emitters->particle;
}

// --- internal ---

static void Emitter_Step(World *world, double dt, double time)
{
    if (!world->emitters)
        return;
    float fdt = (float)dt;

    SolEmitters *sys = world->emitters;

    int write = 0;
    for (int i = 0; i < sys->emitter_count; i++)
    {
        Emitter *e = &sys->emitter[i];

        // Detach if the followed entity died — let ttl drain naturally
        if (!(world->masks[e->followId] & BITC(HAS_ACTIVE)))
        {
            e->followId = 0;
            e->inf      = false;
            if (e->ttl <= 0)
                e->ttl = e->particle.ttl; // fade window = one particle lifetime
        }

        if (!e->inf)
        {
            e->ttl -= fdt;
            if (e->ttl <= 0)
                continue;
        }

        // Position update
        if (e->followId)
            e->pos = Sol_Xform_GetPos(world, e->followId);
        else
            e->pos = vecAdd(e->pos, vecSca(e->vel, fdt));

        // Spawn over time with sub-step to avoid clumping on lag
        e->accumulator += fdt;
        if (e->rate > 0)
        {
            while (e->accumulator >= e->rate)
            {
                for (int b = 0; b < e->rateBurst || b < 1; b++)
                {
                    Particle *p         = Particle_Activate(sys, e);
                    float     lagOffset = e->accumulator / fdt;
                    p->pos              = vecAdd(p->pos, vecSca(p->vel, -lagOffset * fdt));
                }
                e->accumulator -= e->rate;
            }
        }

        sys->emitter[write++] = *e;
    }
    sys->emitter_count = write;
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
            vec3s vel = p->vel;
            if (p->followId)
                vel = vecAdd(vel, Sol_Physx_GetVel(world, p->followId));
            p->pos = vecAdd(p->pos, vecSca(vel, fdt));
            p->rot += p->rotspeed * dt;
        }

        s->particle[write++] = *p;
    }
    s->particle_count = write;
}

static Particle *Particle_Activate(SolEmitters *s, Emitter *e)
{
    Sol_Realloc((void **)&s->particle, s->particle_count, &s->particle_capacity, sizeof(Particle));

    Particle *p = &s->particle[s->particle_count++];
    memcpy(p, &e->particle, sizeof(Particle));

    p->scaleout = p->randScaleout ? (rand() % 100) * p->scaleout * 0.01f : p->scaleout;
    p->ttl      = p->randLife ? (rand() % 100) * p->ttl * 0.01f : p->ttl;
    p->span     = p->ttl;
    p->ttl += p->delay;
    p->scale = p->randScale ? (rand() % 100) * p->scale * 0.01f : p->scale;

    float theta = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;
    float phi   = acosf(2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f);
    float speed = p->speed;
    p->vel.x    = sinf(phi) * cosf(theta) * speed;
    p->vel.y    = sinf(phi) * sinf(theta) * speed;
    p->vel.z    = cosf(phi) * speed;
    p->pos      = vecAdd(e->pos, vecSca(p->vel, p->offset));

    Sol_Debug_Add("Particles", (float)s->particle_count);
    return p;
}

static void Particle_Draw(World *world, double dt, double time)
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
            case PARTICLE_SPARKFRONT:
                textureId = SOL_TEXTURE_SHOCKPARTICLE;
                quadKind  = QUADKIND_SPRITE_FRONT;
                break;
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
                quadKind  = QUADKIND_SPRITE;
                break;
            case PARTICLE_SHOCK_ADD:
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
