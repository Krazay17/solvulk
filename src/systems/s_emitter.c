#include "world.h"
#include "sol_math.h"
#include "render/render.h"

const Emitter emitter_kinds[EMITTERKIND_COUNT] = {
    [EMITTERKIND_SPHERE_BURST_FRACTAL] =
        {
            .kind       = EMITKIND_SPHERE,
            .burst      = 1,
            .rate       = 0.1f,
            .ttl        = 5.0f,
            .p_kind     = PARTICLE_FRACTAL,
            .p_lifespan = 5.0f,
            .p_scale    = 5.0f,
            .p_color    = {1, 1, 1, 1},
            .p_speed    = 2.0f,
        },
};

const Particle particle_kinds[PARTICLE_COUNT] = {
    [PARTICLE_FRACTAL] =
        {
            .scale    = 1.0f,
            .lifespan = 1.0f,
            .color    = {1, 1, 1, 1},
            .speed    = 1.0f,
        },
};

const struct ParticleConf
{
    float fadein;
    float fadeout;
    float scalein;
    float scaleout;
} particle_conf_kinds[PARTICLE_COUNT] = {
    [PARTICLE_FRACTAL] =
        {
            .fadein   = 0.5f,
            .fadeout  = 0.5f,
            .scalein  = 0.5f,
            .scaleout = 0.5f,
        },
    [PARTICLE_SPHERE] =
        {
            .fadein   = 0.5f,
            .fadeout  = 0.5f,
            .scalein  = 0.5f,
            .scaleout = 0.5f,
        },
};
const QuadKind particle_quad[PARTICLE_COUNT] = {
    [PARTICLE_FRACTAL] = QUADKIND_FRACTAL_PYRAMID,
    [PARTICLE_SPHERE]  = QUADKIND_SPRITE,
};

const SolTextureId particle_texture[PARTICLE_COUNT] = {
    [PARTICLE_SPHERE] = SOL_TEXTURE_CLOUDPARTICLE,
};

static inline vec3s RandomVel_Sphere(float speed)
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

static inline vec3s RandomVel_Cone(float speed, vec3s direction, float max_angle_radians)
{
    // 1. Uniform sample inside the canonical cone along +Z
    float psi     = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159265f;
    float cos_max = cosf(max_angle_radians);
    float cos_phi = 1.0f - ((float)rand() / (float)RAND_MAX) * (1.0f - cos_max);
    float sin_phi = sqrtf(1.0f - cos_phi * cos_phi);

    vec3s local_dir = {.x = sin_phi * cosf(psi), .y = sin_phi * sinf(psi), .z = cos_phi};

    // 2. Normalize base direction W
    float dir_len = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    vec3s W       = {direction.x / dir_len, direction.y / dir_len, direction.z / dir_len};

    // 3. Construct tangent frame (U, V) perpendicular to W
    vec3s ref = (fabsf(W.z) < 0.999f) ? (vec3s){0.0f, 0.0f, 1.0f} : (vec3s){1.0f, 0.0f, 0.0f};

    // Cross product ref x W
    vec3s U     = {ref.y * W.z - ref.z * W.y, ref.z * W.x - ref.x * W.z, ref.x * W.y - ref.y * W.x};
    float u_len = sqrtf(U.x * U.x + U.y * U.y + U.z * U.z);
    U.x /= u_len;
    U.y /= u_len;
    U.z /= u_len;

    // Cross product W x U
    vec3s V = {W.y * U.z - W.z * U.y, W.z * U.x - W.x * U.z, W.x * U.y - W.y * U.x};

    // 4. Rotate local vector to target direction and scale by speed
    vec3s vel = {.x = (local_dir.x * U.x + local_dir.y * V.x + local_dir.z * W.x) * speed,
                 .y = (local_dir.x * U.y + local_dir.y * V.y + local_dir.z * W.y) * speed,
                 .z = (local_dir.x * U.z + local_dir.y * V.z + local_dir.z * W.z) * speed};

    return vel;
}

static inline void Particle_Spawn(SlEmitter *single, vec3s pos, Emitter emitter)
{
    Particle base = particle_kinds[emitter.p_kind];
    base.kind     = emitter.p_kind;
    base.pos      = pos;
    base.color    = glms_vec4_mul(base.color, emitter.p_color);
    base.scale *= emitter.p_scale;
    base.speed *= emitter.p_speed;
    base.lifespan *= emitter.p_lifespan;

    for (int i = 0; i < emitter.burst; i++)
    {
        Particle p = base;
        switch (emitter.kind)
        {
        case EMITKIND_SPHERE:
            p.vel = RandomVel_Sphere(p.speed);
            break;
        case EMITKIND_CONE:
            p.vel = RandomVel_Cone(p.speed, emitter.dir, emitter.cone);
            break;
        }
        solb_push(single->particles, p);
    }
}

static inline void Particle_Update(World *world, SlEmitter *single, float fdt)
{
    int count = solb_count(single->particles);
    int write = 0;
    for (int i = 0; i < count; i++)
    {
        Particle *particle = &single->particles[i];
        particle->elapsed += fdt;
        if (particle->elapsed >= particle->lifespan)
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
        emitter->accum += fdt;
        float rate = emitter->rate;
        if (emitter->rate > 0)
        {
            while (emitter->accum >= rate)
            {
                emitter->accum -= rate;
                Particle_Spawn(single, emitter->pos, *emitter);
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
        Particle p = single->particles[i];

        *Sol_Render_GetNextQuad(particle_quad[p.kind]) = (QuadSSBO){
            .pos       = {p.pos.x, p.pos.y, p.pos.z, p.scale},
            .rect      = {0, 0, 1, 1},
            .color     = p.color,
            .textureId = particle_texture[p.kind],
            .uv        = {0, 0, 1, 1},
        };
    }
}

void Sol_Emitter_Push(World *world, vec3s pos, Emitter emitter)
{
    SlEmitter *single = world->singles[SINGLE_EMITTER];
    emitter.pos       = pos;

    Particle_Spawn(single, pos, emitter);

    if (emitter.ttl > 0)
        solb_push(single->emitters, emitter);
}
