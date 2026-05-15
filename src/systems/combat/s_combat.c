#include "sol_core.h"

#include "combat_i.h"

static void Combat_Step(World *world, double dt, double time);

void Sol_Combat_Init(World *world)
{
    WAddStep(world) = Combat_Step;
}

static void Combat_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e = &world->events->event[i];
        if (e->kind != EVENT_HIT)
            continue;

        for (int i = 0; i < e->as.hit.buffcount; i++)
        {
            Sol_Buff_Add(world, e->as.hit.target,  e->as.hit.buffs[i], &e->as.hit);
        }

        switch (e->as.hit.kind)
        {
        case DAMAGEKIND_FIRE:
            Sol_Emitter_Add(world, (Emitter){
                                       .burst = 50,
                                       .pos   = e->as.hit.pos,
                                       .particle =
                                           (Particle){
                                               .ttl   = 2.5f,
                                               .scale = 0.5f,
                                               .color = (vec4s){1, 1, 1, 1},
                                               .kind  = PARTICLE_GFLAME,
                                               .speed = 5.0f,
                                           },
                                   });
            break;
        }

        Sol_Vital_Damage(world, e->as.hit.target, e->as.hit.damage);
    }
}