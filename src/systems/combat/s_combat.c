#include "sol_core.h"

typedef struct CompContact
{
    bool dealsdamage;
} CompContact;

static void Contact_Step(World *world, double dt, double time);
static void Combat_Step(World *world, double dt, double time);

void Sol_Combat_Init(World *world)
{
    WAddStep(world) = Contact_Step;
    WAddStep(world) = Combat_Step;
    world->contacts = calloc(MAX_ENTS, sizeof(CompContact));
}

void Sol_Contact_Add(World *world, int id, CombatDesc desc)
{
    world->masks[id] |= HAS_CONTACT;
    CompContact *c = &world->contacts[id];
    c->dealsdamage = desc.dealsdamage;
}

static void Contact_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e = &world->events->event[i];

        switch (e->kind)
        {
        case EVENT_COLLISION:
            int proj, other;
            if (world->masks[e->as.collision.entA] & HAS_CONTACT)
            {
                proj  = e->as.collision.entA;
                other = e->as.collision.entB;
            }
            else if (world->masks[e->as.collision.entB] & HAS_CONTACT)
            {
                proj  = e->as.collision.entB;
                other = e->as.collision.entA;
            }
            else
                continue;
            if (!(world->masks[other] & HAS_VITAL))
                continue;

                Sol_Event_Add(world, (SolEvent){
                    .as.hit = {
                        .source = Sol_Owner_GetOwner(world, proj),
                        .target = other,
                    }
                });

            Sol_Emitter_Add(world, (Emitter){
                                       .burst = 50,
                                       .pos   = e->as.collision.pos,
                                       .particle =
                                           (Particle){
                                               .ttl   = 2.5f,
                                               .scale = 0.5f,
                                               .color = (vec4s){1, 1, 1, 1},
                                               .kind  = PARTICLE_GFLAME,
                                               .speed = 5.0f,
                                           },
                                   });
            Sol_Buff_Add(world, other, (BuffDesc){.kind = BUFFKIND_FIRE, .duration = 10.0f});
            break;
        }
    }
}

static void Combat_Step(World *world, double dt, double time)
{
}