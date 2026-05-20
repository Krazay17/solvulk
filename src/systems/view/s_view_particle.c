#include "sol_core.h"

static SolTextureId texture_map[] = {
    [PARTICLE_FIRE] = SOL_TEXTURE_FIREPARTICLE,
    [PARTICLE_BLOOD]  = SOL_TEXTURE_BLOODPARTICLE,
    [PARTICLE_CLOUD]  = SOL_TEXTURE_CLOUDPARTICLE,
    [PARTICLE_SHOCK] = SOL_TEXTURE_SHOCKPARTICLE,
};

void Sol_View_Particle_Draw(World *world, double dt, double time)
{
    u32 count = Sol_Emitter_GetParticleCount(world);
    if (count == 0)
        return;
    Particle *particles = Sol_Emitter_GetParticles(world);

    for (int i = 0; i < count; i++)
    {
        Particle *p           = &particles[i];
        float     t           = p->ttl / p->span;
        float     visualScale = p->scale;

        if ((1.0f - t) < p->scalein)
            visualScale = p->scale * ((1.0f - t) / p->scalein);
        if (t < p->scaleout)
            visualScale = p->scale * (t / p->scaleout);

        vec4s color = p->color.a > 0 ? p->color : (vec4s){1, 1, 1, 1};

        if (p->kind == PARTICLE_ORB)
        {
            SphereSSBO *sphereSSBO = Sol_Render_GetNext_Sphere(true);

            sphereSSBO->pos[0]   = p->pos.x;
            sphereSSBO->pos[1]   = p->pos.y;
            sphereSSBO->pos[2]   = p->pos.z;
            sphereSSBO->pos[3]   = visualScale;
            sphereSSBO->color[0] = color.r;
            sphereSSBO->color[1] = color.g;
            sphereSSBO->color[2] = color.b;
            sphereSSBO->color[3] = color.a;
        }
        else
        {
            QuadSSBO *quadSSBO    = Sol_Render_GetNext_Sprite(p->kind != PARTICLE_BLOOD);
            quadSSBO->pos[0]      = p->pos.x;
            quadSSBO->pos[1]      = p->pos.y;
            quadSSBO->pos[2]      = p->pos.z;
            quadSSBO->pos[3]      = visualScale;
            quadSSBO->color[0]    = color.r;
            quadSSBO->color[1]    = color.g;
            quadSSBO->color[2]    = color.b;
            quadSSBO->color[3]    = color.a;
            quadSSBO->uv[0]       = 0;
            quadSSBO->uv[1]       = 0;
            quadSSBO->uv[2]       = 1.0f;
            quadSSBO->uv[3]       = 1.0f;
            quadSSBO->rotation[0] = p->rot;
            quadSSBO->rotation[1] = 0;
            quadSSBO->rotation[2] = 0;
            quadSSBO->rotation[3] = 1.0f;
            quadSSBO->textureId   = texture_map[p->kind];
        }
    }
}
