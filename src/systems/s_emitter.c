#include "world.h"
#include "sol_math.h"
#include "render/render.h"

const Emitter emitter_kinds[EMITTERKIND_COUNT] = {
    [EMITTERKIND_SPHERE] =
        {
            .ttl         = 0.0f,
            .burst       = 1,
            .rate        = 0.5f,
            .speed       = 0.0f,
            .kind        = EMITKIND_STILL,
            .p_kind      = PARTICLE_SPHERE,
            .p_lifespan  = 1.0f,
            .p_scale     = 1.0f,
            .p_color     = {1, 0, 0, 1},
            .alpha_curve = CURVE_SMOOTH_INOUT,
            .scale_curve = CURVE_QUICKIN_SLOWOUT,
        },
    [EMITTERKIND_SPHERE_BURST_FRACTAL] =
        {
            .ttl         = 0.3f,
            .burst       = 10,
            .rate        = 0.1f,
            .speed       = 5.0f,
            .kind        = EMITKIND_SPHERE,
            .p_kind      = PARTICLE_FRACTAL,
            .p_lifespan  = 1.0f,
            .p_scale     = 3.0f,
            .p_color     = {1, 1, 1, 1},
            .alpha_curve = CURVE_EASE_OUT,
            .scale_curve = CURVE_QUICKIN_SLOWOUT,
        },
    [EMITTERKIND_SMOKE_BURST] =
        {
            .ttl         = 0.3f,
            .burst       = 25,
            .rate        = 0.1f,
            .speed       = 3.0f,
            .kind        = EMITKIND_SPHERE,
            .p_kind      = PARTICLE_SMOKE,
            .p_lifespan  = 1.0f,
            .p_scale     = 2.0f,
            .p_color     = {1, 0, 0, 1},
            .alpha_curve = CURVE_EASE_OUT,
            .scale_curve = CURVE_QUICKIN_SLOWOUT,
        },
    [EMITTERKIND_BURST] =
        {
            .ttl         = 0.3f,
            .burst       = 20,
            .rate        = 0.1f,
            .speed       = 3.0f,
            .kind        = EMITKIND_SPHERE,
            .p_kind      = PARTICLE_SMOKE,
            .p_lifespan  = 1.0f,
            .p_scale     = 1.0f,
            .p_color     = {1, 1, 1, 1},
            .alpha_curve = CURVE_EASE_OUT,
            .scale_curve = CURVE_QUICKIN_SLOWOUT,
        },
};

enum RenderKind
{
    RENDERKIND_QUAD,
    RENDERKIND_SPHERE,
};
const u32 particle_pipekind[PARTICLE_COUNT] = {
    [PARTICLE_FRACTAL] = RENDERKIND_QUAD,
    [PARTICLE_SMOKE]   = RENDERKIND_QUAD,
    [PARTICLE_SPHERE]  = RENDERKIND_SPHERE,
    [PARTICLE_SPARK]  = RENDERKIND_QUAD,
    [PARTICLE_PLASMA] = RENDERKIND_SPHERE,
};

const u32 particle_renderkind[PARTICLE_COUNT] = {
    [PARTICLE_FRACTAL] = QUADKIND_FRACTAL_PYRAMID,
    [PARTICLE_SMOKE]   = QUADKIND_SPRITE,
    [PARTICLE_SPHERE]  = SPHEREKIND_BASIC,
    [PARTICLE_PLASMA] = SPHEREKIND_PLASMA,
};

const SolTextureId particle_texture[PARTICLE_COUNT] = {
    [PARTICLE_SMOKE]  = SOL_TEXTURE_CLOUDPARTICLE,
    [PARTICLE_SPARK]  = SOL_TEXTURE_SHOCKPARTICLE,
    [PARTICLE_BLOOD]  = SOL_TEXTURE_BLOODPARTICLE,
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

static inline void Particle_Spawn(SlEmitter *single, Emitter emitter)
{
    Particle p = {
        .pos         = emitter.pos,
        .kind        = emitter.p_kind,
        .color       = emitter.p_color,
        .scale       = emitter.p_scale,
        .lifespan    = emitter.p_lifespan,
        .alpha_curve = emitter.alpha_curve,
        .scale_curve = emitter.scale_curve,
    };

    for (int i = 0; i < emitter.burst; i++)
    {
        Particle inst = p;
        switch (emitter.kind)
        {
        case EMITKIND_SPHERE:
            inst.vel = RandomVel_Sphere(emitter.speed);
            break;
        case EMITKIND_CONE:
            inst.vel = RandomVel_Cone(emitter.speed, emitter.dir, emitter.cone);
            break;
        default:
            break;
        }
        solb_push(single->particles, inst);
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
    SlEmitter *single = Sol_Comp_Get(world, 0, SlEmitter);

    int count = solb_count(single->emitters);
    int write = 0;
    for (int i = 0; i < count; i++)
    {
        Emitter *emitter = &single->emitters[i];
        emitter->pos     = vecAdd(emitter->pos, vecSca(emitter->vel, fdt));
        emitter->ttl -= fdt;
        emitter->accum += fdt;
        while (emitter->accum >= emitter->rate)
        {
            emitter->accum = emitter->rate > 0.0f ? emitter->accum - emitter->rate : -1.0f;
            Particle_Spawn(single, *emitter);
        }
        if (emitter->ttl > 0.0f)
            single->emitters[write++] = *emitter;
    }
    solb_set_count(single->emitters, write);

    Particle_Update(world, single, fdt);
}

void Particle_Draw(World *world)
{
    SlEmitter *single = Sol_Comp_Get(world, 0, SlEmitter);

    int count = solb_count(single->particles);
    for (int i = 0; i < count; i++)
    {
        Particle p        = single->particles[i];
        float t           = p.elapsed / p.lifespan;
        float scale       = EvaluateCurve(p.scale_curve, t);
        float alpha       = EvaluateCurve(p.alpha_curve, t);
        float final_scale = p.scale;
        final_scale *= scale;
        vec4s final_color = p.color;
        final_color.a     = final_color.a * alpha;

        switch (particle_pipekind[p.kind])
        {
        case RENDERKIND_QUAD:
            *Sol_Render_GetNextQuad(particle_renderkind[p.kind]) = (QuadSSBO){
                .pos       = {p.pos.x, p.pos.y, p.pos.z, final_scale},
                .rect      = {0, 0, 1, 1},
                .color     = final_color,
                .textureId = particle_texture[p.kind],
                .uv        = {0, 0, 1, 1},
            };
            break;
        case RENDERKIND_SPHERE:
            sollog(particle_renderkind[p.kind]);
            *Sol_Render_GetNextSphere(particle_renderkind[p.kind]) = (SphereSSBO){
                .pos   = {p.pos.x, p.pos.y, p.pos.z, final_scale},
                .color = final_color,
            };
            break;
        }
    }
}

void Sol_Emitter_Spawn(World *world, EmitterKind kind, vec3s pos)
{
    Emitter e = emitter_kinds[kind];
    e.pos     = pos;
    Sol_Emitter_Push(world, &e, 1);
}

void Sol_Emitter_Push(World *world, Emitter *emitters, int count)
{
    SlEmitter *single = Sol_Comp_Get(world, 0, SlEmitter);
    for (int i = 0; i < count; i++)
    {
        Particle_Spawn(single, emitters[i]);
        if (emitters[i].ttl > 0)
            solb_push(single->emitters, emitters[i]);
    }
}

void Sol_Emitter_PushE(World *world, Emitter *emitters, int count, vec3s pos, vec3s vel, vec3s dir)
{
    SlEmitter *single = Sol_Comp_Get(world, 0, SlEmitter);
    for (int i = 0; i < count; i++)
    {
        emitters[i].pos = pos;
        emitters[i].vel = vel;
        emitters[i].dir = dir;
        Particle_Spawn(single, emitters[i]);
        if (emitters[i].ttl > 0)
            solb_push(single->emitters, emitters[i]);
    }
}

Emitter *Sol_Emitter_Next(World *world, EmitterKind kind)
{
    SlEmitter *single = Sol_Comp_Get(world, 0, SlEmitter);
    Emitter *e        = solb_next(single->emitters);
    *e                = emitter_kinds[kind];
    e->accum          = e->rate;
    return e;
}
