#include "emitter.h"
#include "sol_core.h"

void Emitter_Add(World *world, EmitterDesc e)
{
    SolEmitters *sys = world->emitters;

    if (sys->count >= sys->capacity)
    {
        sys->capacity *= 2;
        sys->emitter = realloc(sys->emitter, sizeof(Emitter) * sys->capacity);
    }
    u32 index = sys->count++;

    Emitter *em = &sys->emitter[index];
    *em         = (Emitter){
        .emitterKind  = e.emitterKind,
        .particleKind = e.particleKind,
        .pos          = e.pos,
        .life         = 5.0f,
        .rate         = 10.0f,
        .vel          = (vec3s){0, 5, 0},
    };

    em->particles      = sys->particle_pool;
    em->particleOffset = sys->particle_cursor;
    em->particleCount  = EMITTER_PARTICLES;

    sys->particle_cursor += EMITTER_PARTICLES;

    Sol_Debug_Add("Emitters", sys->count);
    assert(sys->particle_cursor < MAX_PARTICLES && "Particle Pool Overflow!");

    for (int i = 0; i < EMITTER_PARTICLES; i++)
    {
        int       idx = em->particleOffset + i;
        Particle *p   = &sys->particle_pool[idx];

        p->pos   = e.pos;
        p->scale = (rand() % 100) * 0.001f;
        p->vel   = (vec3s){((float)(rand() % 100) / 50.0f) - 1.0f, ((float)(rand() % 100) / 50.0f) - 1.0f, ((float)(rand() % 100) / 50.0f) - 1.0f};
        p->life  = 1.0f;
    }
}

void Emitter_Remove(World *world, int index)
{
    world->emitters->emitter[index].life = -1.0f;
    // Emitter emitter                 = world->emitters->emitter[index];
    // world->emitters->emitter[index] = world->emitters->emitter[--world->emitters->count];
}

void Emitter_Init(World *world)
{
    world->emitters           = malloc(sizeof(SolEmitters));
    world->emitters->count    = 0;
    world->emitters->capacity = MAX_EMITTERS;
    world->emitters->emitter  = calloc(world->emitters->capacity, sizeof(Emitter));

    world->emitters->particle_pool   = calloc(MAX_PARTICLES, sizeof(Particle));
    world->emitters->particle_cursor = 0;
}

void Emitter_Tick(World *world, double dt, double time)
{
    if (!world->emitters)
        return;

    float        fdt = (float)dt;
    SolEmitters *sys = world->emitters;

    int write = 0;
    for (int i = 0; i < sys->count; i++)
    {
        sys->emitter[i].life -= fdt;
        if (sys->emitter[i].life < 0)
            continue;

        sys->emitter[i].pos = vecAdd(sys->emitter[i].pos, vecSca(sys->emitter[i].vel, fdt));

        int start = sys->emitter[i].particleOffset;
        int end   = start + sys->emitter[i].particleCount;
        for (int j = start; j < end; j++)
        {
            Particle *p = &sys->particle_pool[j];
            p->pos      = vecAdd(p->pos, vecSca(p->vel, fdt));
        }

        sys->emitter[write++] = sys->emitter[i];
    }
    world->emitters->count = write;
}

void Emitter_Draw(World *world, double dt, double time)
{
    float        fdt = (float)dt;
    SolEmitters *sys = world->emitters;
    for (int i = 0; i < sys->count; i++)
    {
        int start = sys->emitter[i].particleOffset;
        int end   = start + sys->emitter[i].particleCount;
        for (int j = start; j < end; j++)
        {
            Particle *p = &sys->particle_pool[j];
            Sol_Submit_Sphere((vec4s){p->pos.x, p->pos.y, p->pos.z, p->scale}, (vec4s){0, 255, 55, 255});
        }
    }
}
