#include "sol_core.h"

typedef struct CompContact
{
    SolHit hit;
    u32    bounces;
} CompContact;

static void Contact_Step(World *world, double dt, double time);

void Sol_Contact_Init(World *world)
{
    WAddStep(world) = Contact_Step;
    world->contacts = calloc(MAX_ENTS, sizeof(CompContact));
}

void Sol_Contact_Add(World *world, int id, ContactDesc desc)
{
    world->masks[id] |= HAS_CONTACT;
    CompContact *c = &world->contacts[id];
    c->hit         = desc.hit;
    c->bounces     = desc.bounces;
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

            CompContact *c   = &world->contacts[proj];
            SolHit       hit = c->hit;
            hit.target       = other;
            hit.pos          = e->as.collision.pos;
            hit.dir          = vecSub(Sol_Xform_GetPos(world, other), Sol_Xform_GetPos(world, proj));

            Sol_Event_Add(world, (SolEvent){.kind = EVENT_HIT, .as.hit = hit});

            if (c->bounces < 1)
                Sol_Destroy_Ent(world, proj);
            else
                c->bounces--;
            break;
        }
    }
}
