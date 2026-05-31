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

        case PARTICLE_BLOOD: {
            QuadSSBO *o  = Sol_Render_GetNext_Quad(QUADKIND_SPRITE);
            o->pos       = (vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale};
            o->color     = color;
            o->uv        = (vec4s){0, 0, 1, 1};
            o->rot       = (vec4s){p->rot, 0, 0, 1};
            o->textureId = texture_map[p->kind];
        }
        break;

        default: {
            QuadSSBO *o  = Sol_Render_GetNext_Quad(QUADKIND_SPRITE_ADD);
            o->pos       = (vec4s){p->pos.x, p->pos.y, p->pos.z, visualScale};
            o->color     = color;
            o->uv        = (vec4s){0, 0, 1, 1};
            o->rot       = (vec4s){p->rot, 0, 0, 1};
            o->textureId = texture_map[p->kind];
        }
        break;
        }
    }
}
