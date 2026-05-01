#include "emitter.h"
#include "sol_core.h"

static EmitterConfig emitter_kinds[EMITTER_COUNT] = {
    [EMITTER_FOUNTAIN] = {.duration = 5.0f, .rate = 25, .spread = 1.0f},
};

static Particle particle_pool[MAX_PARTICLES] = {0};
static u32      particleCursor               = 0;

void Emitter_Add(World *world, EmitterDesc e)
{
    u32 index = world->emitters->count++;
    if (world->emitters->count >= world->emitters->capacity)
    {
        world->emitters->capacity *= 2;
        world->emitters->emitter = realloc(world->emitters->emitter, sizeof(Emitter) * world->emitters->capacity);
    }
    // EmitterConfig base  = emitter_kinds[e.emitterKind];
    // EmitterConfig final = {
    //     .rate = e.overrides.rate > 0 ? e.overrides.rate : base.rate,
    // };
    Emitter *em        = &world->emitters->emitter[index];
    *em                = (Emitter){.rate = 10.0f,.emitterKind = e.emitterKind, .pos = e.pos, .life = 5.0f, .vel = (vec3s){0, 5, 0}};
    em->particles      = particle_pool;
    em->particleOffset = particleCursor;
    em->particleCount  = EMITTER_PARTICLES;
    particleCursor += EMITTER_PARTICLES;
    assert(particleCursor < MAX_PARTICLES);
    Sol_Debug_Add("EmitterPos", e.pos.y);
    Sol_Debug_Add("Emitters", world->emitters->count);
}

void Emitter_Remove(World *world, int index)
{
    Emitter emitter                 = world->emitters->emitter[index];
    world->emitters->emitter[index] = world->emitters->emitter[--world->emitters->count];
}

void Emitter_Init(World *world)
{
    world->emitters           = malloc(sizeof(SolEmitters));
    world->emitters->count    = 0;
    world->emitters->capacity = MAX_EMITTERS;

    world->emitters->emitter = calloc(world->emitters->capacity, sizeof(Emitter));
}

void Emitter_Tick(World *world, double dt, double time)
{
    if (!world->emitters)
        return;

    float    fdt     = (float)dt;
    int      count   = world->emitters->count;
    Emitter *emitter = world->emitters->emitter;
    int      write   = 0;
    for (int i = 0; i < count; i++)
    {
        emitter[i].life -= fdt;
        if (emitter[i].life >= 0)
            emitter[write++] = emitter[i];

        emitter[i].pos = vecAdd(emitter[i].pos, vecSca(emitter[i].vel, fdt));

        int start = emitter[i].particleOffset;
        int end   = start + emitter[i].particleCount;
        for (int j = start; j < end; j++)
        {
            emitter[i].particles[j].pos =
                vecAdd(emitter[i].particles[j].pos, vecSca(emitter[i].particles[j].vel, fdt));
        }
    }
    world->emitters->count = write;
}

void Emitter_Draw(World *world, double dt, double time)
{
    float    fdt     = (float)dt;
    int      count   = world->emitters->count;
    Emitter *emitter = world->emitters->emitter;
    for (int i = 0; i < count; i++)
    {
        int start = emitter[i].particleOffset;
        int end   = start + emitter[i].particleCount;
        for (int j = start; j < end; j++)
        {
            Sol_Submit_Sphere(
                (vec4s){
                    emitter[i].particles[j].pos.x,
                    emitter[i].particles[j].pos.y,
                    emitter[i].particles[j].pos.z,
                    1.0f,
                },
                (vec4s){.r = 0, .g = 255, .b = 55, .a = 255});
        }
    }
}
