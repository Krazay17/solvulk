#include "sol_core.h"

typedef struct CompCombat
{
    bool dealsdamage;
} CompCombat;

void Sol_Combat_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Combat_Step;
    world->combats                         = calloc(MAX_ENTS, sizeof(CompCombat));
}

void Sol_Combat_Add(World *world, int id, CombatDesc desc)
{
    world->masks[id] |= HAS_COMBAT;
    CompCombat *c  = &world->combats[id];
    c->dealsdamage = desc.dealsdamage;
}

void Sol_Combat_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e = &world->events->event[i];

        switch (e->kind)
        {
        case EVENT_COLLISION:
            int proj, other;
            if (world->masks[e->as.collision.entA] & HAS_COMBAT)
            {
                proj  = e->as.collision.entA;
                other = e->as.collision.entB;
            }
            else if (world->masks[e->as.collision.entB] & HAS_COMBAT)
            {
                proj  = e->as.collision.entB;
                other = e->as.collision.entA;
            }
            else
                continue;
            if (!(world->masks[other] & HAS_VITAL))
                continue;
            Sol_Emitter_Add(world, (Emitter){
                                       .burst = 50,
                                       .pos   = e->as.collision.pos,
                                       .particle =
                                           (Particle){
                                               .ttl   = 2.5f,
                                               .scale = 1.25f,
                                               .color = (vec4s){1, 1, 1, 1},
                                               .kind  = PARTICLE_GFLAME,
                                               .speed = 15.0f,
                                           },
                                   });
            Sol_Buff_Add(world, other, (BuffDesc){.kind = BUFFKIND_FIRE, .duration = 10.0f});
        }
    }
}