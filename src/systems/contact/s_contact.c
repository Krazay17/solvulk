#include "sol_core.h"

typedef struct CompContact
{
    ImpactList impacts;
    u32        bounces;
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
    c->impacts     = desc.impacts;
    c->bounces     = desc.bounces;
}

static void Contact_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e   = &world->events->event[i];
        vec3s     pos = e->as.collision.pos;
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

            CompContact *c = &world->contacts[proj];

            for (int i = 0; i < c->impacts.impactCount; i++)
            {
                Impact *impact = &c->impacts.impacts[i];

                if (impact->kind == IMPACT_DIRECT)
                {
                    if (world->owners[proj].ownerId != other)
                    {
                        impact->hit.target = other;
                        impact->hit.pos    = e->as.collision.pos;
                        impact->hit.dir    = vecSub(Sol_Xform_GetPos(world, other), Sol_Xform_GetPos(world, proj));
                        Sol_Event_Add(world, (SolEvent){.kind = EVENT_HIT, .as.hit = impact->hit});
                        impact->emitter.pos = pos;
                        Sol_Emitter_Add(world, impact->emitter);
                    }
                }
                else if (impact->kind == IMPACT_AOE)
                {
                    SolRayResult results[256];

                    int hits = Sol_SphereCast(world, (SolRay){.pos = pos}, impact->radius, results, 256);
                    for (int i = 0; i < hits; i++)
                    {
                        SolRayResult result = results[i];
                        if ((world->masks[result.entId] & HAS_VITAL) && world->owners[proj].ownerId != result.entId)
                        {
                            impact->hit.target = result.entId;
                            impact->hit.pos    = result.pos;
                            impact->hit.dir    = vecSub(Sol_Xform_GetPos(world, other), Sol_Xform_GetPos(world, proj));
                            Sol_Event_Add(world, (SolEvent){.kind = EVENT_HIT, .as.hit = impact->hit});
                        }
                    }
                }
            }

            if (c->bounces < 1)
                Sol_Destroy_Ent(world, proj);
            else
                c->bounces--;
            break;
        }
    }
}
