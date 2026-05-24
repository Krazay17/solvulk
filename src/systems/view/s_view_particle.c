#include "sol_core.h"

static SolTextureId texture_map[] = {
    [PARTICLE_FIRE]  = SOL_TEXTURE_FIREPARTICLE,
    [PARTICLE_BLOOD] = SOL_TEXTURE_BLOODPARTICLE,
    [PARTICLE_CLOUD] = SOL_TEXTURE_CLOUDPARTICLE,
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
        Particle *p = &particles[i];

        if (p->ttl > p->span)
            continue;

        float t           = p->ttl / p->span;
        float visualScale = p->scale;

        if ((1.0f - t) < p->scalein)
            visualScale = p->scale * ((1.0f - t) / p->scalein);
        if (t < p->scaleout)
            visualScale = p->scale * (t / p->scaleout);

        vec4s color = p->color.a > 0 ? p->color : (vec4s){1, 1, 1, 1};

        switch (p->kind)
        {
        case PARTICLE_ORB: {
            SphereSSBO *o = Sol_Render_GetNext_Sphere(true);
            o->pos[0]     = p->pos.x;
            o->pos[1]     = p->pos.y;
            o->pos[2]     = p->pos.z;
            o->pos[3]     = visualScale;
            o->color[0]   = color.r;
            o->color[1]   = color.g;
            o->color[2]   = color.b;
            o->color[3]   = color.a;
        }
        break;

        case PARTICLE_FIREBALL: {
            SphereSSBO *o = Sol_Render_GetNext_Fireball();
            o->pos[0]     = p->pos.x;
            o->pos[1]     = p->pos.y;
            o->pos[2]     = p->pos.z;
            o->pos[3]     = visualScale;
            o->color[0]   = color.r;
            o->color[1]   = color.g;
            o->color[2]   = color.b;
            o->color[3]   = color.a;
        }
        break;

        case PARTICLE_BLOOD: {
            QuadSSBO *o    = Sol_Render_GetNext_Sprite(false);
            o->pos[0]      = p->pos.x;
            o->pos[1]      = p->pos.y;
            o->pos[2]      = p->pos.z;
            o->pos[3]      = visualScale;
            o->color[0]    = color.r;
            o->color[1]    = color.g;
            o->color[2]    = color.b;
            o->color[3]    = color.a;
            o->uv[0]       = 0;
            o->uv[1]       = 0;
            o->uv[2]       = 1.0f;
            o->uv[3]       = 1.0f;
            o->rotation[0] = p->rot;
            o->rotation[1] = 0;
            o->rotation[2] = 0;
            o->rotation[3] = 1.0f;
            o->textureId   = texture_map[p->kind];
        }
        break;

        default: {
            QuadSSBO *o    = Sol_Render_GetNext_Sprite(true);
            o->pos[0]      = p->pos.x;
            o->pos[1]      = p->pos.y;
            o->pos[2]      = p->pos.z;
            o->pos[3]      = visualScale;
            o->color[0]    = color.r;
            o->color[1]    = color.g;
            o->color[2]    = color.b;
            o->color[3]    = color.a;
            o->uv[0]       = 0;
            o->uv[1]       = 0;
            o->uv[2]       = 1.0f;
            o->uv[3]       = 1.0f;
            o->rotation[0] = p->rot;
            o->rotation[1] = 0;
            o->rotation[2] = 0;
            o->rotation[3] = 1.0f;
            o->textureId   = texture_map[p->kind];
        }
        break;
        
        }
    }
}
