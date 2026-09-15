#include "world.h"
#include "sol_math.h"
#include "render/render.h"

static inline vec3s RandomVelocity(float speed)
{
    float theta = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;
    float phi   = acosf(2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f);

    vec3s vel = {
        .x = sinf(phi) * cosf(theta) * speed,
        .y = sinf(phi) * sinf(theta) * speed,
        .z = cosf(phi) * speed,
    };

    return vel;
}

const Emitter emitter_default = {
    .particle_kind = PARTICLE_FRACTAL,
    .rate          = 5,
    .ttl           = 10.0f,
};

const Particle particle_default = {
    .color    = {1, 1, 1, 1},
    .ttl      = 5.0f,
    .fadein   = 0.5f,
    .fadeout  = 0.5f,
    .scalein  = 0.5f,
    .scaleout = 0.5f,
    .speed    = 2.0f,
};

const Particle particle_kinds[PARTICLE_COUNT] = {
    [PARTICLE_FRACTAL] =
        {
            .kind     = PARTICLE_FRACTAL,
            .color    = {1, 1, 1, 1},
            .ttl      = 5.0f,
            .fadein   = 0.5f,
            .fadeout  = 0.5f,
            .scalein  = 0.5f,
            .scaleout = 0.5f,
            .speed    = 2.0f,
            .scale    = 1.0f,
        },
    [PARTICLE_SPHERE] =
        {
            .kind     = PARTICLE_SPHERE,
            .color    = {1, 1, 1, 1},
            .ttl      = 5.0f,
            .fadein   = 0.5f,
            .fadeout  = 0.5f,
            .scalein  = 0.5f,
            .scaleout = 0.5f,
            .speed    = 2.0f,
            .scale    = 1.0f,
        },
};
const QuadKind particle_quad[PARTICLE_COUNT] = {
    [PARTICLE_FRACTAL] = QUADKIND_FRACTAL_PYRAMID,
    [PARTICLE_SPHERE]  = QUADKIND_SPRITE,
};
const SolTextureId particle_texture[PARTICLE_COUNT] = {
    [PARTICLE_SPHERE] = SOL_TEXTURE_CLOUDPARTICLE,
};

static inline Particle *Particle_Make(SlEmitter *single, Particle particle)
{
}

inline void Emitter_Make(SlEmitter *single, Emitter emitter)
{
    for (int i = 0; i < emitter.burst; i++)
    {
        Particle particle = particle_kinds[emitter.particle_kind];
        particle.pos      = emitter.pos;
        particle.vel      = RandomVelocity(particle.speed);
        solb_push(single->particles, particle);
    }

    if (emitter.ttl > 0)
        solb_push(single->emitters, emitter);
}

static void Particle_Update(World *world, SlEmitter *single, float fdt)
{
    int count = solb_count(single->particles);
    int write = 0;
    for (int i = 0; i < count; i++)
    {
        Particle *particle = &single->particles[i];
        particle->ttl -= fdt;
        if (particle->ttl <= 0.0f)
            continue;
        particle->pos = vecAdd(particle->pos, vecSca(particle->vel, fdt));

        single->particles[write++] = *particle;
    }

    solb_set_count(single->particles, write);
}

void Emitter_Update(World *world)
{
    float fdt         = world->fdt;
    SlEmitter *single = world->singles[SINGLE_EMITTER];
    int count         = solb_count(single->emitters);

    int write = 0;
    for (int i = 0; i < count; i++)
    {
        Emitter *emitter = &single->emitters[i];
        emitter->pos     = vecAdd(emitter->pos, vecSca(emitter->vel, fdt));
        emitter->ttl -= fdt;
        if (emitter->ttl <= 0.0f)
            continue;
        emitter->accumulator += fdt;
        if (emitter->rate > 0)
        {
            while (emitter->accumulator >= emitter->rate)
            {
                emitter->accumulator -= emitter->rate;
                for (int j = 0; j < emitter->burst; j++)
                {
                    Particle particle = particle_kinds[emitter->particle_kind];
                    particle.pos      = emitter->pos;
                    particle.vel      = RandomVelocity(particle.speed);
                    solb_push(single->particles, particle);
                }
            }
        }
        single->emitters[write++] = *emitter;
    }
    solb_set_count(single->emitters, write);

    Particle_Update(world, single, fdt);
}

void Particle_Draw(World *world)
{
    float fdt         = world->fdt;
    SlEmitter *single = world->singles[SINGLE_EMITTER];
    int count         = solb_count(single->particles);

    int write = 0;
    for (int i = 0; i < count; i++)
    {
        Particle particle = single->particles[i];

        *Sol_Render_GetNextQuad(particle_quad[particle.kind]) = (QuadSSBO){
            .pos       = {particle.pos.x, particle.pos.y, particle.pos.z, particle.scale},
            .rect      = {0, 0, 1, 1},
            .rot       = GLMS_VEC4_ZERO,
            .color     = particle.color,
            .textureId = particle_texture[particle.kind],
            .uv        = {0, 0, 1, 1},
            .extra     = (vec4s){1.0f, 0.015f, 1.0f, 0},
            .type      = QUADTYPE_FACECAM,
        };
    }
}

void Sol_Emitter_Push(World *world, Emitter emitter)
{
    SlEmitter *single = world->singles[SINGLE_EMITTER];
    Emitter_Make(single, emitter);
}

void Sol_Particle_Burst(World *world, Particle particle, int count)
{
    SlEmitter *single = world->singles[SINGLE_EMITTER];
    for (int i = 0; i < count; i++)
    { 
        particle.vel = RandomVelocity(particle.speed);
        solb_push(single->particles, particle);
    }
}